#pragma once

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "openal/al_buffer.h"
#include "openal/al_filter.h"
#include "openal/al_reverb.h"

#include <memory>
#include <vector>

using namespace godot;

class ALSourceHandle;

class ALSource : public Node3D
{
    GDCLASS(ALSource, Node3D);

private:
    std::vector<std::unique_ptr<ALSourceHandle>> sources;

protected:
    static void _bind_methods();

    ALuint buffer_handle = 0;

    TypedArray<AudioStream> streams;

    std::vector<ALBuffer> decoded_buffers;
    std::vector<Ref<AudioStream>> decoded_streams;

    // Buffers dropped from decoded_buffers while a source was still playing them, deleted once nothing uses them - alDeleteBuffers fails on an attached buffer
    std::vector<ALBuffer> retired_buffers;

    void retire_buffer(ALBuffer &&buffer);

    void release_retired_buffers();

    std::vector<Ref<AudioStream>> pending_streams;

    WorkerThreadPool::TaskID decode_task_id = WorkerThreadPool::INVALID_TASK_ID;
    Ref<AudioStream> decoding_stream;
    ALBuffer decoding_buffer;
    bool decode_succeeded = false;
    bool play_requested = false;

    int last_played_index = -1;

    bool ensure_stream_decode_started();

    static void decode_stream_task(void *userdata);

    void poll_decode_task();

    int pick_stream_index() const;

    bool start_playing();

    float gain = 1.0f;
    float pitch = 1.0f;
    bool looping = false;
    bool autoplay = false;

    float pitch_randomness = 1.0f;

    float volume_randomness_db = 0.0f;

    bool playback_no_repeat = true;

    bool reverb_only = false;

    // The fullReverb last passed to update_filter
    bool full_reverb = true;

    // !(reverb_only || ALManager's reverb_only), as last applied to the sources
    bool dry_output_enabled = true;

    // Recalculates dry_output_enabled, returning whether it changed
    bool update_dry_output();

    // Applies the direct filter (or the silence filter while reverb-only) and the reverb send
    void apply_filters(ALSourceHandle &source) const;

    virtual void configure_source(ALSourceHandle &source) = 0;

    // A play() waiting for its streams to decode, which isn't playing yet. Not public, so it stays out of the generated docs
    bool is_play_requested() const
    {
        return play_requested;
    }

    friend struct VABoundsPlayback;

public:
    ALSource();
    ~ALSource();

    void _ready() override;

    void _process(double delta) override;

    ALFilter filter;

    ALReverbEffect *effect = nullptr;

    virtual bool play();

    virtual void stop();

    bool is_playing() const;

    // Whether the sound is sent to a reverb effect (grouped or the listener's). A raytraced source outside the bounds has none
    bool has_reverb() const
    {
        return effect != nullptr;
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
        ALSource::stop();
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

protected:
    std::vector<std::unique_ptr<ALSourceHandle>> &get_sources()
    {
        return sources;
    }
};
