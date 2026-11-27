// AI-created

#include "fmod_objects.h"

#include "fmod_backend.h"

#include <algorithm>
#include <cmath>

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

void FMODVoice::set_gain(float gain)
{
    if (has_channel())
        call(va_fmod.FMOD_Channel_SetVolume(channel, gain), "Channel::setVolume");
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

void FMODVoice::set_filters(const VALowPass &direct_low_pass, VAReverbEffect *reverb_effect, const VALowPass &send)
{
    if (!has_channel() || !eq || direct == direct_low_pass)
        return;

    direct = direct_low_pass;
    set_eq(eq, to_decibels(direct_low_pass.gain_lf), to_decibels(mid(direct_low_pass)), to_decibels(direct_low_pass.gain_hf));
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

    if (has_channel() && eq)
        call(va_fmod.FMOD_Channel_RemoveDSP(channel, eq), "Channel::removeDSP");

    if (has_channel())
        call(va_fmod.FMOD_Channel_Stop(channel), "Channel::stop");

    // System::release already freed these with the device
    if (backend->is_initialised())
    {
        if (eq)
            FMODBackend::check(va_fmod.FMOD_DSP_Release(eq), "DSP::release");

        if (stream_sound)
            FMODBackend::check(va_fmod.FMOD_Sound_Release(stream_sound), "Sound::release");
    }

    released = true;
    channel = nullptr;
    eq = nullptr;
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
