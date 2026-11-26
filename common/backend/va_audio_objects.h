// AI-created

#pragma once

#include <godot_cpp/variant/vector3.hpp>

#include <vaudio.h>

#include <cstdint>
#include <memory>

#include "va_audio_types.h"

class VAReverbEffect;

// One playing instance of a sound inside the audio backend (an OpenAL source, an FMOD channel, ...). Each player node owns one, or a few while one-shots overlap. Deleting it stops and frees it
class VAVoice
{
public:
    virtual ~VAVoice() = default;

    virtual VAVoiceState get_state() const = 0;

    // Stopped and not looping, so it can be deleted
    virtual bool is_finished() const = 0;

    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;

    // The sound's length in seconds, or 0 if unknown (e.g. a stream)
    virtual float get_length() const = 0;

    // Seconds into the sound
    virtual float get_playback_position() const = 0;
    virtual void set_playback_position(float seconds) = 0;

    virtual bool get_looping() const = 0;
    virtual void set_looping(bool looping) = 0;

    virtual void set_gain(float gain) = 0;
    virtual void set_pitch(float pitch) = 0;

    // Whether the position is relative to the listener rather than the world
    virtual void set_relative(bool relative) = 0;

    virtual void set_spatialised(bool spatialised) = 0;
    virtual void set_position(const godot::Vector3 &position) = 0;
    virtual void set_velocity(const godot::Vector3 &velocity) = 0;
    virtual void set_reference_distance(float distance) = 0;
    virtual void set_max_distance(float distance) = 0;
    virtual void set_rolloff(float rolloff) = 0;

    // Sets the direct path's low pass, and sends the voice through the send low pass into reverb_effect (nullptr for no reverb)
    virtual void set_filters(const VALowPass &direct, VAReverbEffect *reverb_effect, const VALowPass &send) = 0;

    // The reverb send's level (0-1) after distance attenuation, or -1 when not sending or the backend doesn't report it. For tests
    virtual float get_reverb_send_level() const
    {
        return -1.0f;
    }
};

// Raw PCM pushed into the backend and played by its voice, e.g. for VAStreamSource. Its voice starts playing straight away. Deleting it closes the stream
class VAStream
{
public:
    virtual ~VAStream() = default;

    virtual VAVoice *get_voice() = 0;

    // Copies the bytes and queues them to play after what's already queued. Safe to call from any thread
    virtual void enqueue(const uint8_t *data, int length) = 0;
};

// Decoded audio the backend can play. Must outlive the voices created from it
class VASound
{
public:
    virtual ~VASound() = default;

    // The length in seconds
    virtual float get_length() const = 0;

    // nullptr when the backend is out of voices
    virtual std::unique_ptr<VAVoice> create_voice(bool spatialised) = 0;
};

// A reverb effect that voices send into. Each VAWorld owns one for the listener's reverb, plus one per grouped EAX
class VAReverbEffect
{
public:
    virtual ~VAReverbEffect() = default;

    // False if the backend failed to create it
    virtual bool is_valid() const = 0;

    // Applies the raytraced reverb. gain is the effect's output volume, and the pans point where the reflections and late reverb come from, relative to the listener
    virtual void set_params(const VAEAXReverb &eax, float gain, const godot::Vector3 &reflections_pan, const godot::Vector3 &late_reverb_pan) = 0;
};

// A microphone or other input device. Deleting it closes the device
class VACaptureDevice
{
public:
    virtual ~VACaptureDevice() = default;

    virtual void start() = 0;
    virtual void stop() = 0;

    // Hands what's been captured since the last poll to the data callback
    virtual void poll() = 0;
};
