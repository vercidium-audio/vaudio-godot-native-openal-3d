// AI-created

#pragma once

#include <memory>
#include <optional>

#include "backend/va_audio_objects.h"
#include "backend/va_stream_queue.h"
#include "fmod_api.h"
#include "fmod_rolloff.h"

class FMODBackend;

// An FMOD channel, with a THREE_EQ DSP as its muffling filter. Created paused, so it stays Initial until played. Mirrors the C# plugin's FMODVoice
class FMODVoice : public VAVoice
{
private:
    FMODBackend *backend;

    // Cleared when FMOD reports the channel finished or stolen, which get_state does too
    mutable FMOD_CHANNEL *channel;

    // The direct path's muffling filter
    FMOD_DSP *eq = nullptr;

    float sample_rate;
    float length;

    // A stream's voice owns its sound and always loops, as FMOD keeps reading the stream
    FMOD_SOUND *stream_sound;
    bool is_stream;

    // FMOD keeps this pointer rather than copying the curve, so it lives as long as the voice
    FMOD_VECTOR rolloff_points[FMODRolloff::POINT_COUNT];

    // Set once the channel and DSPs are freed, by the destructor or by the backend's shutdown
    bool released = false;

    bool played = false;
    bool looping = false;
    bool relative;
    bool spatialised;
    godot::Vector3 position;
    godot::Vector3 velocity;
    float reference_distance = 1.0f;
    float max_distance = 1000.0f;
    float rolloff = 1.0f;
    std::optional<VALowPass> direct;

    bool has_channel() const;

    // A channel that finished, or was stolen for a more important sound, is invalid. That's how FMOD reports a stopped voice, so it isn't an error
    bool call(FMOD_RESULT result, const char *name) const;

    // Adds a THREE_EQ at the input end of the chain
    bool add_eq(FMOD_DSP *&dsp);

    // Releases the DSP if it couldn't be added
    bool add_to_channel(FMOD_DSP *&dsp);

    void set_eq(FMOD_DSP *dsp, float low, float mid, float high);

    void apply_mode();
    void apply_3d_attributes();

public:
    FMODVoice(FMODBackend *backend, FMOD_CHANNEL *channel, float sample_rate, float length, bool spatialised, FMOD_SOUND *stream_sound);
    ~FMODVoice() override;

    FMODVoice(const FMODVoice &) = delete;
    FMODVoice &operator=(const FMODVoice &) = delete;

    // Stops the channel and frees its DSPs and stream sound. Called by the backend's shutdown for voices that outlive it
    void release();

    // Also called by the backend when the distance model changes. FMOD ignores the min/max distance with a custom curve
    void apply_rolloff();

    VAVoiceState get_state() const override;
    bool is_finished() const override;

    void play() override;
    void pause() override;
    void stop() override;

    float get_length() const override
    {
        return length;
    }

    float get_playback_position() const override;
    void set_playback_position(float seconds) override;

    bool get_looping() const override
    {
        return looping;
    }

    void set_looping(bool value) override;

    void set_gain(float gain) override;
    void set_pitch(float pitch) override;
    void set_relative(bool value) override;
    void set_spatialised(bool value) override;
    void set_position(const godot::Vector3 &value) override;
    void set_velocity(const godot::Vector3 &value) override;
    void set_reference_distance(float distance) override;
    void set_max_distance(float distance) override;
    void set_rolloff(float value) override;

    void set_filters(const VALowPass &direct_low_pass, VAReverbEffect *reverb_effect, const VALowPass &send) override;
};

// A decoded sound held in FMOD's memory
class FMODSound : public VASound
{
private:
    FMOD_SOUND *sound;

    // The system the sound belongs to, so it isn't released after that system was
    FMOD_SYSTEM *owner;

    int sample_rate;
    float length;

public:
    FMODSound(FMOD_SOUND *sound, FMOD_SYSTEM *owner, int sample_rate, float length) : sound(sound), owner(owner), sample_rate(sample_rate), length(length) {}
    ~FMODSound() override;

    float get_length() const override
    {
        return length;
    }

    std::unique_ptr<VAVoice> create_voice(bool spatialised) override;
};

// A user-created FMOD stream sound, whose read callback pulls from the queue on FMOD's stream thread
class FMODStream : public VAStream
{
private:
    // Destroyed after the voice, which stops FMOD reading it
    VAStreamQueue queue;

    // FMOD's PCM8 is signed, VAPcmFormat's 8-bit formats are unsigned like OpenAL's
    bool unsigned8 = false;

    std::unique_ptr<FMODVoice> voice;

    static FMOD_RESULT F_CALL read_callback(FMOD_SOUND *sound, void *data, unsigned int length);

public:
    // Creates the stream sound and starts playing it, or logs why it couldn't and returns false
    bool create(FMODBackend *backend, VAPcmFormat format, int sample_rate);

    VAVoice *get_voice() override
    {
        return voice.get();
    }

    void enqueue(const uint8_t *data, int length) override
    {
        queue.enqueue(data, length);
    }
};
