// AI-created

#include "openal_objects.h"

#include "openal_backend.h"

#include <vector>

using namespace godot;

bool OpenALVoice::create(ALuint buffer, bool stream)
{
    if (!source.create())
        return false;

    is_stream = stream;
    source.set_buffer(buffer);
    return true;
}

VAVoiceState OpenALVoice::get_state() const
{
    switch (source.get_state())
    {
        case AL_INITIAL: return VAVoiceState::Initial;
        case AL_PLAYING: return VAVoiceState::Playing;
        case AL_PAUSED: return VAVoiceState::Paused;
        default: return VAVoiceState::Stopped;
    }
}

bool OpenALVoice::is_finished() const
{
    return source.is_finished();
}

void OpenALVoice::play()
{
    source.play();
}

void OpenALVoice::pause()
{
    source.pause();
}

void OpenALVoice::stop()
{
    source.stop();
}

// A stream's callback buffer has no fixed length
float OpenALVoice::get_length() const
{
    return is_stream ? 0.0f : source.get_buffer_length();
}

float OpenALVoice::get_playback_position() const
{
    return source.get_seconds_offset();
}

void OpenALVoice::set_playback_position(float seconds)
{
    source.set_seconds_offset(seconds);
}

bool OpenALVoice::get_looping() const
{
    return source.get_looping();
}

void OpenALVoice::set_looping(bool looping)
{
    source.set_looping(looping);
}

void OpenALVoice::set_gain(float gain)
{
    source.set_gain(gain);
}

void OpenALVoice::set_pitch(float pitch)
{
    source.set_pitch(pitch);
}

void OpenALVoice::set_relative(bool relative)
{
    source.set_relative(relative);
}

void OpenALVoice::set_spatialised(bool spatialised)
{
    source.set_spatialized(spatialised);
}

void OpenALVoice::set_position(const Vector3 &position)
{
    source.set_position(position);
}

void OpenALVoice::set_velocity(const Vector3 &velocity)
{
    source.set_velocity(velocity);
}

void OpenALVoice::set_reference_distance(float distance)
{
    source.set_reference_distance(distance);
}

void OpenALVoice::set_max_distance(float distance)
{
    source.set_max_distance(distance);
}

void OpenALVoice::set_rolloff(float rolloff)
{
    source.set_rolloff_factor(rolloff);
}

// ALFilter::set_gain takes an absolute gain_hf and converts it to AL_LOWPASS_GAINHF's relative value itself
static void update_filter(ALFilter &filter, const VALowPass &low_pass)
{
    if (filter.is_valid())
        filter.set_gain(low_pass.gain_lf, low_pass.gain_hf);
    else
        filter.create(low_pass.gain_lf, low_pass.gain_hf);
}

void OpenALVoice::set_filters(const VALowPass &direct, VAReverbEffect *reverb_effect, const VALowPass &send)
{
    update_filter(direct_filter, direct);
    update_filter(send_filter, send);

    // Only one backend runs at a time, so every reverb effect is an OpenALReverbEffect here
    OpenALReverbEffect *effect = static_cast<OpenALReverbEffect *>(reverb_effect);

    source.set_direct_filter(direct_filter.get_handle());
    source.set_reverb_send(effect && effect->is_valid() ? effect->get_slot_handle() : 0, send_filter.get_handle());
}

OpenALSound::~OpenALSound()
{
    OpenALBackend::delete_sound_buffer(handle);
}

float OpenALSound::get_length() const
{
    return al_buffer_length(handle);
}

std::unique_ptr<VAVoice> OpenALSound::create_voice(bool spatialised)
{
    if (handle == 0)
        return nullptr;

    auto voice = std::make_unique<OpenALVoice>();

    if (!voice->create(handle, false))
        return nullptr;

    voice->set_relative(!spatialised);
    voice->set_spatialised(spatialised);
    return voice;
}

bool OpenALStream::create(VAPcmFormat format, int frequency)
{
    if (!buffer.create(format, frequency))
        return false;

    if (!voice.create(buffer.get_handle(), true))
        return false;

    voice.play();
    return true;
}

void OpenALStream::enqueue(const uint8_t *data, int length)
{
    buffer.enqueue(data, length);
}

OpenALReverbEffect::OpenALReverbEffect()
{
    effect.create();
}

void OpenALReverbEffect::set_params(const VAEAXReverb &eax, float gain, const Vector3 &reflections_pan, const Vector3 &late_reverb_pan)
{
    VAEAXReverbParams params = CopyReverbParams(&eax);

    params.effectSlotGain = gain;

    params.reflectionsPan[0] = reflections_pan.x;
    params.reflectionsPan[1] = reflections_pan.y;
    params.reflectionsPan[2] = reflections_pan.z;

    params.lateReverbPan[0] = late_reverb_pan.x;
    params.lateReverbPan[1] = late_reverb_pan.y;
    params.lateReverbPan[2] = late_reverb_pan.z;

    effect.set_params(params);
}

bool OpenALCaptureDevice::open(const String &device_name, VAPcmFormat format, int frequency, int buffer_frames, VACaptureCallback on_data)
{
    int bytes_per_frame = va_pcm_bytes_per_frame(format);

    // ALCaptureDevice reports bytes, the interface reports frames
    return device.open(device_name, OpenALBackend::to_al(format), frequency, buffer_frames, [on_data = std::move(on_data), bytes_per_frame](const uint8_t *data, int bytes)
    {
        on_data(data, bytes / bytes_per_frame);
    });
}
