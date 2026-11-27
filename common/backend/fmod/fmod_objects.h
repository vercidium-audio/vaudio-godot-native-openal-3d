// AI-created

#pragma once

#include <godot_cpp/variant/string.hpp>

#include <memory>
#include <optional>
#include <vector>

#include "backend/va_audio_backend_base.h"
#include "backend/va_audio_objects.h"
#include "backend/va_stream_queue.h"
#include "fmod_api.h"
#include "fmod_rolloff.h"

class FMODBackend;
class FMODReverbEffect;

// An FMOD channel, with a THREE_EQ DSP as its muffling filter. Created paused, so it stays Initial until played. Mirrors the C# plugin's FMODVoice
class FMODVoice : public VAVoice
{
private:
    FMODBackend *backend;

    // Cleared when FMOD reports the channel finished or stolen, which get_state does too
    mutable FMOD_CHANNEL *channel;

    // The direct path's muffling filter
    FMOD_DSP *eq = nullptr;

    // The reverb send and its filter, before eq. Created when the voice first sends to a reverb effect
    FMOD_DSP *send = nullptr;
    FMOD_DSP *send_eq = nullptr;
    int send_return_id = -1;
    FMODReverbEffect *send_effect = nullptr;
    float send_level = -1.0f;
    VALowPass pre_send = VALowPass::full();

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
    float gain = 1.0f;

    bool has_channel() const;

    // A channel that finished, or was stolen for a more important sound, is invalid. That's how FMOD reports a stopped voice, so it isn't an error
    bool call(FMOD_RESULT result, const char *name) const;

    // Adds a THREE_EQ at the input end of the chain
    bool add_eq(FMOD_DSP *&dsp);

    // Adds the SEND and its EQ the first time the voice sends to a reverb effect. Most voices never do
    bool add_send();

    // Releases the DSP if it couldn't be added
    bool add_to_channel(FMOD_DSP *&dsp);

    void set_eq(FMOD_DSP *dsp, float low, float mid, float high);

    // The same attenuation the voice's own rolloff curve gives the direct path. A relative voice's distance is from the listener, and an unspatialised one (FMOD's 2D mode) isn't attenuated
    float send_distance_gain() const;

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

    void set_filters(const VALowPass &direct_low_pass, VAReverbEffect *reverb_effect, const VALowPass &send_low_pass) override;

    float get_reverb_send_level() const override;

    // Called whenever the gain or effect changes, and every frame by the backend, since the voice's or listener's position may have moved
    void update_send_level();
};

// A reverb bus: a channel group whose RETURN DSP collects every voice's SEND, through an SFXREVERB DSP and, once it's panned, a PAN DSP. The group's volume is the effect's gain. Mirrors the C# plugin's FMODReverbEffect
class FMODReverbEffect : public VAReverbEffect
{
private:
    // The system the group belongs to, so nothing is released after that system was
    FMOD_SYSTEM *owner = nullptr;

    FMOD_CHANNELGROUP *group = nullptr;
    FMOD_DSP *reverb = nullptr;
    FMOD_DSP *return_dsp = nullptr;
    FMOD_DSP *pan = nullptr;
    int return_id = -1;

    // The last value set per SFXREVERB parameter, so unchanged ones aren't set again
    float reverb_params[FMOD_DSP_SFXREVERB_DRYLEVEL + 1];

    float last_gain = -1.0f;
    godot::Vector3 last_pan;

    bool owner_alive() const;
    bool add_dsp(FMOD_DSP_TYPE type, FMOD_DSP *&dsp);
    void set_reverb_param(FMOD_DSP_SFXREVERB param, float value);
    void set_pan(const godot::Vector3 &direction);
    void release();

public:
    FMODReverbEffect();
    ~FMODReverbEffect() override;

    FMODReverbEffect(const FMODReverbEffect &) = delete;
    FMODReverbEffect &operator=(const FMODReverbEffect &) = delete;

    bool is_valid() const override;
    void set_params(const VAEAXReverb &eax, float gain, const godot::Vector3 &reflections_pan, const godot::Vector3 &late_reverb_pan) override;

    // The RETURN DSP's ID, which voices' SEND DSPs target
    int get_return_id() const
    {
        return return_id;
    }
};

// Records a capture driver into a looping user-created sound, and poll() hands what's been recorded since the last poll to the data callback. Mirrors the C# plugin's FMODCaptureDevice
class FMODCaptureDevice : public VACaptureDevice
{
private:
    // The system the sound belongs to. System::release frees the sound and stops recording
    FMOD_SYSTEM *owner;
    int driver;
    godot::String device_name;
    VAPcmFormat format;
    VACaptureCallback on_data;
    unsigned int length_frames;

    FMOD_SOUND *sound;
    unsigned int read_frame = 0;
    bool recording = false;
    std::vector<uint8_t> scratch;

    bool owner_alive() const;

public:
    FMODCaptureDevice(FMOD_SYSTEM *owner, int driver, const godot::String &device_name, VAPcmFormat format, VACaptureCallback on_data, FMOD_SOUND *sound, unsigned int length_frames) : owner(owner), driver(driver), device_name(device_name), format(format), on_data(std::move(on_data)), length_frames(length_frames), sound(sound) {}
    ~FMODCaptureDevice() override;

    FMODCaptureDevice(const FMODCaptureDevice &) = delete;
    FMODCaptureDevice &operator=(const FMODCaptureDevice &) = delete;

    void start() override;
    void stop() override;
    void poll() override;
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
