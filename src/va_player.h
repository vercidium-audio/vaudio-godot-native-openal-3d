// AI-created

#pragma once

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "backend/va_audio_objects.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

using namespace godot;

// One of a player's voices, with what it plays. The sound and stream are declared first, so they're destroyed after the voice that uses them
struct VAPlayerVoice
{
    // Shared with the player's decoded sounds, so a sound removed from streams lives until its last voice finishes
    std::shared_ptr<VASound> sound;

    // Owns its voice
    std::unique_ptr<VAStream> stream;

    std::unique_ptr<VAVoice> owned_voice;

    VAVoice *get() const
    {
        return stream ? stream->get_voice() : owned_voice.get();
    }
};

class VAPlayer : public Node3D
{
    GDCLASS(VAPlayer, Node3D);

protected:
    static void _bind_methods();

    std::vector<VAPlayerVoice> voices;

    TypedArray<AudioStream> streams;

    std::vector<std::shared_ptr<VASound>> decoded_sounds;
    std::vector<Ref<AudioStream>> decoded_streams;

    std::vector<Ref<AudioStream>> pending_streams;

    WorkerThreadPool::TaskID decode_task_id = WorkerThreadPool::INVALID_TASK_ID;
    Ref<AudioStream> decoding_stream;
    std::vector<int16_t> decoding_pcm;
    int decoding_sample_rate = 0;
    bool decode_succeeded = false;
    bool play_requested = false;
    float requested_start_time = 0.0f;
    bool stream_paused = false;

    // Moves one sound to this many seconds in. A looping sound wraps around, and one that doesn't loop stops if it's past the end
    static void seek_voice(VAVoice &voice, float time);

    int last_played_index = -1;

    bool ensure_stream_decode_started();

    static void decode_stream_task(void *userdata);

    void poll_decode_task();

    int pick_stream_index() const;

    bool start_playing(float start_time);

    // Pauses or resumes the sound to match stream_paused
    void apply_paused(VAVoice &voice) const;

    // Whether a sound is playing or paused
    bool has_sound() const;

    float gain = 1.0f;
    float pitch = 1.0f;
    bool looping = false;
    bool autoplay = false;

    float pitch_randomness = 1.0f;

    float volume_randomness_db = 0.0f;

    bool playback_no_repeat = true;

    bool reverb_only = false;

    // The muffling last passed to update_filter, if it ever was
    std::optional<VALowPass> muffling;

    // The fullReverb last passed to update_filter
    bool full_reverb = true;

    // !(reverb_only || VAAudioBackend's reverb_only), as last applied to the voices
    bool dry_output_enabled = true;

    // Recalculates dry_output_enabled, returning whether it changed
    bool update_dry_output();

    // Applies the direct filter (or silence while reverb-only) and the reverb send
    void apply_filters(VAVoice &voice) const;

    virtual void configure_voice(VAVoice &voice)
    {
    }

    // For players that play at the listener, e.g. ambience, music, UI
    static void configure_listener_relative_voice(VAVoice &voice);

    // The muffling's absolute LF and relative HF gain, as the raytraced nodes report it, or 0 before update_filter
    float get_muffling_gain_lf_value() const
    {
        return muffling ? muffling->gain_lf : 0.0f;
    }

    float get_muffling_gain_hf_value() const
    {
        return muffling ? muffling->relative_gain_hf() : 0.0f;
    }

    // Releases every voice now. Also called by stop()
    virtual void clear_voices();

    // A play() waiting for its streams to decode, which isn't playing yet. Not public, so it stays out of the generated docs
    bool is_play_requested() const
    {
        return play_requested;
    }

    friend struct VABoundsPlayback;

public:
    VAPlayer();
    ~VAPlayer();

    void _ready() override;

    void _process(double delta) override;

    // The reverb effect each voice sends to, or nullptr for no reverb. Applied by the next update_filter
    VAReverbEffect *reverb_effect = nullptr;

    // The most recently played voice, or nullptr. For tests
    VAVoice *get_last_voice() const
    {
        return voices.empty() ? nullptr : voices.back().get();
    }

    // Plays a new sound, starting start_time seconds in. While stream_paused is set it starts paused, and plays once unpaused
    virtual bool play(float start_time = 0.0f);

    virtual void stop();

    // False while paused, matching AudioStreamPlayer
    bool is_playing() const;

    // Pauses every sound this source is playing, and resumes them from the same time when set back to false. Matches AudioStreamPlayer's stream_paused
    void set_stream_paused(bool value);

    bool get_stream_paused() const
    {
        return stream_paused;
    }

    // Moves every sound this source is playing to this many seconds in, while playing or paused. Matches AudioStreamPlayer's seek()
    virtual void seek(float time);

    // How many seconds into the most recently played sound it is, or 0 if nothing is playing. Matches AudioStreamPlayer's get_playback_position()
    float get_playback_time() const;

    // Whether the sound is sent to a reverb effect (grouped or the listener's). A raytraced source outside the bounds has none
    bool has_reverb() const
    {
        return reverb_effect != nullptr;
    }

    void update_filter(float new_gain, float new_gain_hf, bool fullReverb = false);

    void set_gain(float value);
    void set_pitch(float value);
    void set_looping(bool value);
    void set_autoplay(bool value);

    float get_gain() const
    {
        return gain;
    }

    float get_pitch() const
    {
        return pitch;
    }

    bool get_looping() const
    {
        return looping;
    }

    bool get_autoplay() const
    {
        return autoplay;
    }

    float get_pitch_randomness() const
    {
        return pitch_randomness;
    }

    void set_pitch_randomness(float value)
    {
        pitch_randomness = value;
    }

    float get_volume_randomness_db() const
    {
        return volume_randomness_db;
    }

    void set_volume_randomness_db(float value)
    {
        volume_randomness_db = value;
    }

    bool get_playback_no_repeat() const
    {
        return playback_no_repeat;
    }

    void set_playback_no_repeat(bool value)
    {
        playback_no_repeat = value;
    }

    bool get_reverb_only() const
    {
        return reverb_only;
    }

    // Applied on the next _process
    void set_reverb_only(bool value)
    {
        reverb_only = value;
    }

    // Whether the direct sound is heard, i.e. neither this source's nor the VAWorld's reverb_only is set
    bool is_dry_output_enabled() const
    {
        return dry_output_enabled;
    }

    TypedArray<AudioStream> get_streams() const
    {
        return streams;
    }

    void set_streams(const TypedArray<AudioStream> &value)
    {
        // Matches AudioStreamPlayer.set_stream(), which stops whatever was playing. The base stop(), so it runs while a scene loads without cancelling autoplay
        VAPlayer::stop();
        streams = value;
    }

    // Script-only alias for `streams` matching AudioStreamPlayer3D's single `stream` field (see va_conversion_plugin.cpp's matching remap).
    Ref<AudioStream> get_stream() const
    {
        return streams.is_empty() ? Ref<AudioStream>() : Ref<AudioStream>(streams[0]);
    }

    void set_stream(const Ref<AudioStream> &value)
    {
        TypedArray<AudioStream> value_array;
        value_array.push_back(value);
        set_streams(value_array);
    }

    // Script-only alias for `pitch` matching AudioStreamPlayer3D's `pitch_scale`.
    float get_pitch_scale() const
    {
        return get_pitch();
    }

    void set_pitch_scale(float value)
    {
        set_pitch(value);
    }

    // Script-only alias for `gain` matching AudioStreamPlayer(3D)'s logarithmic `volume_db`; converts via linear_to_db/db_to_linear since `gain` is linear.
    float get_volume_db() const;
    void set_volume_db(float value);
};
