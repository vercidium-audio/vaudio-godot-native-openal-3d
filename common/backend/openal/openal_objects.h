// AI-created

#pragma once

#include "backend/va_audio_backend_base.h"
#include "al_capture_device.h"
#include "al_filter.h"
#include "al_reverb.h"
#include "al_source_handle.h"
#include "al_stream_buffer.h"

// An AL source, with its own direct and send filters
class OpenALVoice : public VAVoice
{
private:
    // Filters and buffers must be deleted after the source that uses them, and members are destroyed in reverse order
    ALFilter direct_filter;
    ALFilter send_filter;
    ALSourceHandle source;

    bool is_stream = false;

public:
    // Creates the AL source, or logs why it couldn't and returns false
    bool create(ALuint buffer, bool stream);

    VAVoiceState get_state() const override;
    bool is_finished() const override;

    void play() override;
    void pause() override;
    void stop() override;

    float get_length() const override;
    float get_playback_position() const override;
    void set_playback_position(float seconds) override;

    bool get_looping() const override;
    void set_looping(bool looping) override;

    void set_gain(float gain) override;
    void set_pitch(float pitch) override;
    void set_relative(bool relative) override;
    void set_spatialised(bool spatialised) override;
    void set_position(const godot::Vector3 &position) override;
    void set_velocity(const godot::Vector3 &velocity) override;
    void set_reference_distance(float distance) override;
    void set_max_distance(float distance) override;
    void set_rolloff(float rolloff) override;

    void set_filters(const VALowPass &direct, VAReverbEffect *reverb_effect, const VALowPass &send) override;
};

// An AL buffer holding a decoded sound
class OpenALSound : public VASound
{
private:
    ALuint handle = 0;

public:
    explicit OpenALSound(ALuint buffer) : handle(buffer) {}
    ~OpenALSound() override;

    float get_length() const override;
    std::unique_ptr<VAVoice> create_voice(bool spatialised) override;
};

// An AL source fed by alBufferCallbackSOFT
class OpenALStream : public VAStream
{
private:
    // Destroyed after the voice, which plays it
    ALStreamBuffer buffer;
    OpenALVoice voice;

public:
    // Creates the buffer and source and starts playing, or logs why it couldn't and returns false
    bool create(VAPcmFormat format, int frequency);

    VAVoice *get_voice() override
    {
        return &voice;
    }

    void enqueue(const uint8_t *data, int length) override;
};

// An EFX EAX reverb effect in an auxiliary effect slot
class OpenALReverbEffect : public VAReverbEffect
{
private:
    ALReverbEffect effect;

public:
    // Silent until set_params writes its slot gain with the first EAX, so nothing plays through OpenAL's default reverb
    OpenALReverbEffect();

    bool is_valid() const override
    {
        return effect.is_valid();
    }

    ALuint get_slot_handle() const
    {
        return effect.get_slot_handle();
    }

    void set_params(const VAEAXReverb &eax, float gain, const godot::Vector3 &reflections_pan, const godot::Vector3 &late_reverb_pan) override;
};

class OpenALCaptureDevice : public VACaptureDevice
{
private:
    ALCaptureDevice device;

public:
    bool open(const godot::String &device_name, VAPcmFormat format, int frequency, int buffer_frames, VACaptureCallback on_data);

    void start() override
    {
        device.start();
    }

    void stop() override
    {
        device.stop();
    }

    void poll() override
    {
        device.update();
    }
};
