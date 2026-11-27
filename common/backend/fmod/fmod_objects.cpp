// AI-created

#include "fmod_objects.h"

#include "fmod_backend.h"

#include <algorithm>
#include <cmath>
#include <iterator>

using namespace godot;

// Matches the EFX reference frequencies the SDK's GainLF and GainHF describe
static constexpr float LOW_CROSSOVER = 250.0f;
static constexpr float HIGH_CROSSOVER = 5000.0f;

// THREE_EQ's lowest gain
static constexpr float SILENCE_DECIBELS = -80.0f;

static float to_decibels(float gain)
{
    return gain <= 0.0f ? SILENCE_DECIBELS : std::clamp(20.0f * std::log10(gain), SILENCE_DECIBELS, 0.0f);
}

// The mid band gets the average of the two, like vaudio-fmod's FMODSound
static float mid(const VALowPass &filter)
{
    return (filter.gain_lf + filter.gain_hf) / 2.0f;
}

FMODVoice::FMODVoice(FMODBackend *backend, FMOD_CHANNEL *channel, float sample_rate, float length, bool spatialised, FMOD_SOUND *stream_sound) : backend(backend), channel(channel), sample_rate(sample_rate), length(length), stream_sound(stream_sound), is_stream(stream_sound != nullptr), relative(!spatialised), spatialised(spatialised)
{
    call(va_fmod.FMOD_Channel_SetLoopCount(channel, -1), "Channel::setLoopCount");
    apply_mode();
    apply_rolloff();
    add_eq(eq);

    backend->add_voice(this);
}

FMODVoice::~FMODVoice()
{
    release();
}

bool FMODVoice::has_channel() const
{
    return !released && channel && backend->is_initialised();
}

bool FMODVoice::call(FMOD_RESULT result, const char *name) const
{
    if (result == FMOD_ERR_INVALID_HANDLE || result == FMOD_ERR_CHANNEL_STOLEN)
    {
        channel = nullptr;
        return false;
    }

    return FMODBackend::check(result, name);
}

VAVoiceState FMODVoice::get_state() const
{
    FMOD_BOOL playing = false;

    if (!has_channel() || !call(va_fmod.FMOD_Channel_IsPlaying(channel, &playing), "Channel::isPlaying") || !playing)
        return VAVoiceState::Stopped;

    FMOD_BOOL paused = false;

    if (!call(va_fmod.FMOD_Channel_GetPaused(channel, &paused), "Channel::getPaused"))
        return VAVoiceState::Stopped;

    if (!paused)
        return VAVoiceState::Playing;

    return played ? VAVoiceState::Paused : VAVoiceState::Initial;
}

bool FMODVoice::is_finished() const
{
    return !looping && get_state() == VAVoiceState::Stopped;
}

void FMODVoice::play()
{
    if (!has_channel())
        return;

    played = true;
    call(va_fmod.FMOD_Channel_SetPaused(channel, false), "Channel::setPaused");
}

void FMODVoice::pause()
{
    if (has_channel())
        call(va_fmod.FMOD_Channel_SetPaused(channel, true), "Channel::setPaused");
}

void FMODVoice::stop()
{
    if (has_channel())
        call(va_fmod.FMOD_Channel_Stop(channel), "Channel::stop");

    channel = nullptr;
}

float FMODVoice::get_playback_position() const
{
    unsigned int frames = 0;
    return has_channel() && call(va_fmod.FMOD_Channel_GetPosition(channel, &frames, FMOD_TIMEUNIT_PCM), "Channel::getPosition") ? frames / sample_rate : 0.0f;
}

void FMODVoice::set_playback_position(float seconds)
{
    if (has_channel())
        call(va_fmod.FMOD_Channel_SetPosition(channel, (unsigned int)(std::max(0.0f, seconds) * sample_rate), FMOD_TIMEUNIT_PCM), "Channel::setPosition");
}

void FMODVoice::set_looping(bool value)
{
    looping = value;
    apply_mode();
}

void FMODVoice::set_gain(float value)
{
    gain = value;

    if (!has_channel())
        return;

    call(va_fmod.FMOD_Channel_SetVolume(channel, value), "Channel::setVolume");
    update_send_level();
}

void FMODVoice::set_pitch(float pitch)
{
    if (has_channel())
        call(va_fmod.FMOD_Channel_SetPitch(channel, pitch), "Channel::setPitch");
}

void FMODVoice::set_relative(bool value)
{
    relative = value;
    apply_mode();
}

void FMODVoice::set_spatialised(bool value)
{
    spatialised = value;
    apply_mode();
}

void FMODVoice::set_position(const Vector3 &value)
{
    position = value;
    apply_3d_attributes();
}

void FMODVoice::set_velocity(const Vector3 &value)
{
    velocity = value;
    apply_3d_attributes();
}

void FMODVoice::set_reference_distance(float distance)
{
    reference_distance = distance;
    apply_rolloff();
}

void FMODVoice::set_max_distance(float distance)
{
    max_distance = distance;
    apply_rolloff();
}

void FMODVoice::set_rolloff(float value)
{
    rolloff = value;
    apply_rolloff();
}

static float remaining_decibels(float gain, float already_applied)
{
    return std::clamp(to_decibels(gain) - to_decibels(already_applied), SILENCE_DECIBELS, 0.0f);
}

// The DSP chain from the input end is send_eq -> send -> eq -> fader. The send comes before the fader and FMOD's 3D rolloff, so it carries the voice's gain and its rolloff curve's distance attenuation itself, and the direct path's silence or extra filtering in eq never reaches it
void FMODVoice::set_filters(const VALowPass &direct_low_pass, VAReverbEffect *reverb_effect, const VALowPass &send_low_pass)
{
    if (!has_channel() || !eq)
        return;

    // The selected backend made every effect, so this is always an FMODReverbEffect
    FMODReverbEffect *effect = static_cast<FMODReverbEffect *>(reverb_effect);
    int return_id = effect && effect->is_valid() ? effect->get_return_id() : -1;

    if (return_id >= 0 && !send && !add_send())
        return_id = -1;

    send_effect = return_id >= 0 ? effect : nullptr;

    if (return_id != send_return_id)
    {
        send_return_id = return_id;
        FMODBackend::check(va_fmod.FMOD_DSP_SetParameterInt(send, FMOD_DSP_SEND_RETURNID, return_id), "DSP::setParameterInt");
    }

    update_send_level();

    // Without a send, eq does all the filtering
    VALowPass new_pre_send = return_id >= 0 ? send_low_pass : VALowPass::full();

    if (pre_send != new_pre_send && send_eq)
    {
        set_eq(send_eq, to_decibels(new_pre_send.gain_lf), to_decibels(mid(new_pre_send)), to_decibels(new_pre_send.gain_hf));
        FMODBackend::check(va_fmod.FMOD_DSP_SetBypass(send_eq, new_pre_send == VALowPass::full()), "DSP::setBypass");
    }

    if (direct == direct_low_pass && pre_send == new_pre_send)
        return;

    direct = direct_low_pass;
    pre_send = new_pre_send;

    // eq comes after send_eq, so it only applies what's left of the direct filter
    set_eq(eq, remaining_decibels(direct_low_pass.gain_lf, new_pre_send.gain_lf), remaining_decibels(mid(direct_low_pass), mid(new_pre_send)), remaining_decibels(direct_low_pass.gain_hf, new_pre_send.gain_hf));
}

bool FMODVoice::add_send()
{
    if (!FMODBackend::check(va_fmod.FMOD_System_CreateDSPByType(backend->get_system(), FMOD_DSP_TYPE_SEND, &send), "System::createDSPByType"))
    {
        send = nullptr;
        return false;
    }

    if (!add_to_channel(send))
        return false;

    if (add_eq(send_eq))
        FMODBackend::check(va_fmod.FMOD_DSP_SetBypass(send_eq, true), "DSP::setBypass");

    return true;
}

float FMODVoice::get_reverb_send_level() const
{
    return send_effect && send ? send_level : -1.0f;
}

void FMODVoice::update_send_level()
{
    if (!has_channel() || !send)
        return;

    float level = std::clamp(gain, 0.0f, 1.0f) * send_distance_gain();

    if (level == send_level)
        return;

    send_level = level;
    FMODBackend::check(va_fmod.FMOD_DSP_SetParameterFloat(send, FMOD_DSP_SEND_LEVEL, level), "DSP::setParameterFloat");
}

float FMODVoice::send_distance_gain() const
{
    if (!spatialised)
        return 1.0f;

    float distance = relative ? position.length() : position.distance_to(backend->get_listener_position());
    return FMODRolloff::curve_gain(backend->get_distance_model(), distance, reference_distance, max_distance, rolloff);
}

void FMODVoice::set_eq(FMOD_DSP *dsp, float low, float mid_gain, float high)
{
    FMODBackend::check(va_fmod.FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_THREE_EQ_LOWGAIN, low), "DSP::setParameterFloat");
    FMODBackend::check(va_fmod.FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_THREE_EQ_MIDGAIN, mid_gain), "DSP::setParameterFloat");
    FMODBackend::check(va_fmod.FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_THREE_EQ_HIGHGAIN, high), "DSP::setParameterFloat");
}

bool FMODVoice::add_eq(FMOD_DSP *&dsp)
{
    if (!FMODBackend::check(va_fmod.FMOD_System_CreateDSPByType(backend->get_system(), FMOD_DSP_TYPE_THREE_EQ, &dsp), "System::createDSPByType"))
        return false;

    FMODBackend::check(va_fmod.FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_THREE_EQ_LOWCROSSOVER, LOW_CROSSOVER), "DSP::setParameterFloat");
    FMODBackend::check(va_fmod.FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_THREE_EQ_HIGHCROSSOVER, HIGH_CROSSOVER), "DSP::setParameterFloat");
    return add_to_channel(dsp);
}

bool FMODVoice::add_to_channel(FMOD_DSP *&dsp)
{
    if (channel && call(va_fmod.FMOD_Channel_AddDSP(channel, FMOD_CHANNELCONTROL_DSP_TAIL, dsp), "Channel::addDSP"))
        return true;

    FMODBackend::check(va_fmod.FMOD_DSP_Release(dsp), "DSP::release");
    dsp = nullptr;
    return false;
}

void FMODVoice::apply_mode()
{
    if (!has_channel())
        return;

    FMOD_MODE mode = (spatialised ? FMOD_3D : FMOD_2D) | (relative ? FMOD_3D_HEADRELATIVE : FMOD_3D_WORLDRELATIVE) | FMOD_3D_CUSTOMROLLOFF | (looping || is_stream ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
    call(va_fmod.FMOD_Channel_SetMode(channel, mode), "Channel::setMode");
}

void FMODVoice::apply_3d_attributes()
{
    if (!has_channel())
        return;

    FMOD_VECTOR fmod_position = FMODBackend::to_fmod(position);
    FMOD_VECTOR fmod_velocity = FMODBackend::to_fmod(velocity);
    call(va_fmod.FMOD_Channel_Set3DAttributes(channel, &fmod_position, &fmod_velocity), "Channel::set3DAttributes");
}

void FMODVoice::apply_rolloff()
{
    if (!has_channel())
        return;

    FMODRolloff::fill(rolloff_points, backend->get_distance_model(), reference_distance, max_distance, rolloff);
    call(va_fmod.FMOD_Channel_Set3DCustomRolloff(channel, rolloff_points, FMODRolloff::POINT_COUNT), "Channel::set3DCustomRolloff");
}

void FMODVoice::release()
{
    if (released)
        return;

    FMOD_DSP *dsps[] = {send_eq, send, eq};

    for (FMOD_DSP *dsp : dsps)
    {
        if (has_channel() && dsp)
            call(va_fmod.FMOD_Channel_RemoveDSP(channel, dsp), "Channel::removeDSP");
    }

    if (has_channel())
        call(va_fmod.FMOD_Channel_Stop(channel), "Channel::stop");

    // System::release already freed these with the device
    if (backend->is_initialised())
    {
        for (FMOD_DSP *dsp : dsps)
        {
            if (dsp)
                FMODBackend::check(va_fmod.FMOD_DSP_Release(dsp), "DSP::release");
        }

        if (stream_sound)
            FMODBackend::check(va_fmod.FMOD_Sound_Release(stream_sound), "Sound::release");
    }

    released = true;
    channel = nullptr;
    eq = nullptr;
    send = nullptr;
    send_eq = nullptr;
    send_effect = nullptr;
    stream_sound = nullptr;
    backend->remove_voice(this);
}

FMODSound::~FMODSound()
{
    // System::release already freed it with the device
    FMODBackend *backend = FMODBackend::get_singleton();

    if (sound && backend && backend->get_system() == owner)
        FMODBackend::check(va_fmod.FMOD_Sound_Release(sound), "Sound::release");
}

std::unique_ptr<VAVoice> FMODSound::create_voice(bool spatialised)
{
    FMODBackend *backend = FMODBackend::get_singleton();

    if (!sound || !backend || backend->get_system() != owner)
        return nullptr;

    return backend->play_sound(sound, sample_rate, length, spatialised);
}

// Short, so pushed audio isn't heard late
static constexpr float DECODE_BUFFER_SECONDS = 0.05f;

FMOD_RESULT F_CALL FMODStream::read_callback(FMOD_SOUND *sound, void *data, unsigned int length)
{
    void *user_data = nullptr;
    va_fmod.FMOD_Sound_GetUserData(sound, &user_data);

    uint8_t *bytes = static_cast<uint8_t *>(data);
    FMODStream *stream = static_cast<FMODStream *>(user_data);

    // FMOD can fill its first buffer before createSound returns, when there's nothing queued yet
    if (!stream)
    {
        std::fill(bytes, bytes + length, (uint8_t)0);
        return FMOD_OK;
    }

    stream->queue.read(bytes, (int)length);

    if (stream->unsigned8)
        FMODBackend::toggle_sign8(bytes, length);

    return FMOD_OK;
}

bool FMODStream::create(FMODBackend *backend, VAPcmFormat format, int sample_rate)
{
    if (sample_rate <= 0)
    {
        VA_ERROR("Can't open the stream because its frequency is ", sample_rate, " Hz. It must be the sample rate of the pushed audio, e.g. 44100");
        return false;
    }

    queue.reset(format);
    unsigned8 = va_pcm_bytes_per_sample(format) == 1;

    FMOD_CREATESOUNDEXINFO exinfo = {};
    exinfo.cbsize = sizeof(exinfo);
    exinfo.numchannels = va_pcm_channels(format);
    exinfo.defaultfrequency = sample_rate;
    exinfo.format = FMODBackend::to_fmod(format);
    exinfo.decodebuffersize = (unsigned int)std::max(256.0f, sample_rate * DECODE_BUFFER_SECONDS);

    // One second, looped forever
    exinfo.length = (unsigned int)(sample_rate * va_pcm_bytes_per_frame(format));
    exinfo.pcmreadcallback = &FMODStream::read_callback;
    exinfo.userdata = this;

    FMOD_MODE mode = FMOD_OPENUSER | FMOD_CREATESTREAM | FMOD_LOOP_NORMAL | FMOD_3D;
    FMOD_SOUND *sound = nullptr;

    if (!FMODBackend::check(va_fmod.FMOD_System_CreateSound(backend->get_system(), nullptr, mode, &exinfo, &sound), "System::createSound"))
        return false;

    voice = backend->play_sound(sound, sample_rate, 0.0f, true, sound);

    if (!voice)
    {
        FMODBackend::check(va_fmod.FMOD_Sound_Release(sound), "Sound::release");
        return false;
    }

    voice->play();
    return true;
}

FMODReverbEffect::FMODReverbEffect()
{
    std::fill(std::begin(reverb_params), std::end(reverb_params), NAN);

    FMODBackend *backend = FMODBackend::get_singleton();

    if (!backend)
        return;

    owner = backend->get_system();

    if (!FMODBackend::check(va_fmod.FMOD_System_CreateChannelGroup(owner, "vaudio reverb", &group), "System::createChannelGroup"))
    {
        group = nullptr;
        return;
    }

    // Silent until set_params sets the gain with the first EAX, so nothing plays through the default reverb
    FMODBackend::check(va_fmod.FMOD_ChannelGroup_SetVolume(group, 0.0f), "ChannelGroup::setVolume");

    // TAIL is the input end, so this builds RETURN -> SFXREVERB -> fader
    if (!add_dsp(FMOD_DSP_TYPE_SFXREVERB, reverb) || !add_dsp(FMOD_DSP_TYPE_RETURN, return_dsp))
    {
        release();
        return;
    }

    set_reverb_param(FMOD_DSP_SFXREVERB_DRYLEVEL, SILENCE_DECIBELS);

    int id = -1;

    if (FMODBackend::check(va_fmod.FMOD_DSP_GetParameterInt(return_dsp, FMOD_DSP_RETURN_ID, &id, nullptr, 0), "DSP::getParameterInt"))
        return_id = id;
    else
        release();
}

FMODReverbEffect::~FMODReverbEffect()
{
    release();
}

bool FMODReverbEffect::owner_alive() const
{
    FMODBackend *backend = FMODBackend::get_singleton();
    return owner && backend && backend->get_system() == owner;
}

bool FMODReverbEffect::add_dsp(FMOD_DSP_TYPE type, FMOD_DSP *&dsp)
{
    if (!FMODBackend::check(va_fmod.FMOD_System_CreateDSPByType(owner, type, &dsp), "System::createDSPByType"))
    {
        dsp = nullptr;
        return false;
    }

    if (FMODBackend::check(va_fmod.FMOD_ChannelGroup_AddDSP(group, FMOD_CHANNELCONTROL_DSP_TAIL, dsp), "ChannelGroup::addDSP"))
        return true;

    FMODBackend::check(va_fmod.FMOD_DSP_Release(dsp), "DSP::release");
    dsp = nullptr;
    return false;
}

bool FMODReverbEffect::is_valid() const
{
    return group && return_id >= 0 && owner_alive();
}

void FMODReverbEffect::set_params(const VAEAXReverb &eax, float gain, const Vector3 &reflections_pan, const Vector3 &late_reverb_pan)
{
    if (!is_valid())
        return;

    // Unit conversions checked against FMOD's presets vs their EFX equivalents. Clamped to FMOD's ranges, which are narrower than EAX's for the ratios
    set_reverb_param(FMOD_DSP_SFXREVERB_DECAYTIME, std::clamp(eax.decayTime * 1000.0f, 100.0f, 20000.0f));
    set_reverb_param(FMOD_DSP_SFXREVERB_EARLYDELAY, std::clamp(eax.reflectionsDelay * 1000.0f, 0.0f, 300.0f));
    set_reverb_param(FMOD_DSP_SFXREVERB_LATEDELAY, std::clamp(eax.lateReverbDelay * 1000.0f, 0.0f, 100.0f));
    set_reverb_param(FMOD_DSP_SFXREVERB_HFREFERENCE, std::clamp(eax.hfReference, 20.0f, 20000.0f));
    set_reverb_param(FMOD_DSP_SFXREVERB_HFDECAYRATIO, std::clamp(eax.decayHFRatio * 100.0f, 10.0f, 100.0f));
    set_reverb_param(FMOD_DSP_SFXREVERB_DIFFUSION, std::clamp(eax.diffusion * 100.0f, 10.0f, 100.0f));
    set_reverb_param(FMOD_DSP_SFXREVERB_DENSITY, std::clamp(eax.density * 100.0f, 10.0f, 100.0f));
    set_reverb_param(FMOD_DSP_SFXREVERB_LOWSHELFFREQUENCY, std::clamp(eax.lfReference, 20.0f, 1000.0f));
    set_reverb_param(FMOD_DSP_SFXREVERB_LOWSHELFGAIN, std::clamp(eax.lowShelfGain, -36.0f, 12.0f));
    set_reverb_param(FMOD_DSP_SFXREVERB_HIGHCUT, std::clamp(eax.highCut, 20.0f, 20000.0f));
    set_reverb_param(FMOD_DSP_SFXREVERB_EARLYLATEMIX, std::clamp(eax.earlyLateMix, 0.0f, 100.0f));

    // GainLF/GainHF also carry the reverb's overall level in OpenAL (band share x energy scale), which HighCut/LowShelfGain don't, so apply the louder one here
    float band_gain = std::max(std::max(eax.gainLF, eax.gainHF), 0.0001f);
    set_reverb_param(FMOD_DSP_SFXREVERB_WETLEVEL, std::clamp(eax.wetLevel + 20.0f * std::log10(band_gain), -80.0f, 20.0f));

    if (gain != last_gain)
    {
        last_gain = gain;
        FMODBackend::check(va_fmod.FMOD_ChannelGroup_SetVolume(group, gain), "ChannelGroup::setVolume");
    }

    // SFXREVERB has one output, so the reflections and late reverb can't be panned separately. VAWorld passes the same pan for both today
    set_pan(late_reverb_pan);
}

void FMODReverbEffect::set_reverb_param(FMOD_DSP_SFXREVERB param, float value)
{
    if (reverb_params[param] == value)
        return;

    reverb_params[param] = value;
    FMODBackend::check(va_fmod.FMOD_DSP_SetParameterFloat(reverb, param, value), "DSP::setParameterFloat");
}

// Like EFX's reverb pan: the direction (+X right, +Z forward) is where the reverb comes from, and the length (0-1) how focused it is there. The PAN DSP is only added once a pan is set, so the listener's effect never has one
void FMODReverbEffect::set_pan(const Vector3 &direction)
{
    if (direction == last_pan)
        return;

    last_pan = direction;

    if (!pan)
    {
        if (!FMODBackend::check(va_fmod.FMOD_System_CreateDSPByType(owner, FMOD_DSP_TYPE_PAN, &pan), "System::createDSPByType"))
        {
            pan = nullptr;
            return;
        }

        // HEAD is the output end, so the pan comes after the fader
        if (!FMODBackend::check(va_fmod.FMOD_ChannelGroup_AddDSP(group, FMOD_CHANNELCONTROL_DSP_HEAD, pan), "ChannelGroup::addDSP"))
        {
            FMODBackend::check(va_fmod.FMOD_DSP_Release(pan), "DSP::release");
            pan = nullptr;
            return;
        }
    }

    float focus = std::clamp(std::sqrt(direction.x * direction.x + direction.z * direction.z), 0.0f, 1.0f);
    float degrees = focus > 0.0f ? std::atan2(direction.x, direction.z) * 180.0f / 3.14159265358979f : 0.0f;

    FMODBackend::check(va_fmod.FMOD_DSP_SetParameterFloat(pan, FMOD_DSP_PAN_2D_DIRECTION, degrees), "DSP::setParameterFloat");
    FMODBackend::check(va_fmod.FMOD_DSP_SetParameterFloat(pan, FMOD_DSP_PAN_2D_EXTENT, 360.0f * (1.0f - focus)), "DSP::setParameterFloat");
}

void FMODReverbEffect::release()
{
    // System::release already freed these with the device
    if (owner_alive())
    {
        for (FMOD_DSP *dsp : {return_dsp, reverb, pan})
        {
            if (!dsp)
                continue;

            if (group)
                FMODBackend::check(va_fmod.FMOD_ChannelGroup_RemoveDSP(group, dsp), "ChannelGroup::removeDSP");

            FMODBackend::check(va_fmod.FMOD_DSP_Release(dsp), "DSP::release");
        }

        if (group)
            FMODBackend::check(va_fmod.FMOD_ChannelGroup_Release(group), "ChannelGroup::release");
    }

    group = nullptr;
    reverb = nullptr;
    return_dsp = nullptr;
    pan = nullptr;
    return_id = -1;
}

FMODCaptureDevice::~FMODCaptureDevice()
{
    if (!sound)
        return;

    stop();

    if (owner_alive())
        FMODBackend::check(va_fmod.FMOD_Sound_Release(sound), "Sound::release");

    sound = nullptr;
}

bool FMODCaptureDevice::owner_alive() const
{
    FMODBackend *backend = FMODBackend::get_singleton();
    return backend && backend->get_system() == owner;
}

void FMODCaptureDevice::start()
{
    if (recording || !sound || !owner_alive())
        return;

    // Recording restarts from the start of the sound
    read_frame = 0;
    recording = FMODBackend::check(va_fmod.FMOD_System_RecordStart(owner, driver, sound, true), "System::recordStart");
}

void FMODCaptureDevice::stop()
{
    if (!recording)
        return;

    // Hand over what was recorded before the stop, like OpenAL's capture buffer
    poll();

    // Polling finds a disconnected device or a released system, which already stopped recording
    if (!recording)
        return;

    recording = false;
    FMODBackend::check(va_fmod.FMOD_System_RecordStop(owner, driver), "System::recordStop");
}

void FMODCaptureDevice::poll()
{
    if (!recording)
        return;

    // System::release stopped the recording and freed the sound
    if (!owner_alive())
    {
        recording = false;
        sound = nullptr;
        return;
    }

    unsigned int record_frame = 0;
    FMOD_RESULT result = va_fmod.FMOD_System_GetRecordPosition(owner, driver, &record_frame);

    // Unplugging isn't a backend error, it just stops the recording
    if (result == FMOD_ERR_RECORD_DISCONNECTED)
    {
        VA_WARN("The capture device '", device_name, "' was disconnected, so capturing stopped");
        recording = false;
        return;
    }

    if (!FMODBackend::check(result, "System::getRecordPosition") || record_frame == read_frame)
        return;

    unsigned int frames = (record_frame + length_frames - read_frame) % length_frames;
    unsigned int bytes_per_frame = (unsigned int)va_pcm_bytes_per_frame(format);
    unsigned int num_bytes = frames * bytes_per_frame;

    void *ptr1 = nullptr;
    void *ptr2 = nullptr;
    unsigned int len1 = 0;
    unsigned int len2 = 0;

    if (!FMODBackend::check(va_fmod.FMOD_Sound_Lock(sound, read_frame * bytes_per_frame, num_bytes, &ptr1, &ptr2, &len1, &len2), "Sound::lock"))
        return;

    if (scratch.size() < num_bytes)
        scratch.resize(num_bytes);

    // The two parts are where the recording wrapped round the end of the sound
    std::copy_n(static_cast<uint8_t *>(ptr1), len1, scratch.data());

    if (ptr2 && len2 > 0)
        std::copy_n(static_cast<uint8_t *>(ptr2), len2, scratch.data() + len1);

    FMODBackend::check(va_fmod.FMOD_Sound_Unlock(sound, ptr1, ptr2, len1, len2), "Sound::unlock");
    read_frame = record_frame;

    if (va_pcm_bytes_per_sample(format) == 1)
        FMODBackend::toggle_sign8(scratch.data(), num_bytes);

    on_data(scratch.data(), (int)frames);
}
