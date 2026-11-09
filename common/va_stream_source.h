#pragma once

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include "openal/al_stream_buffer.h"
#include "va_raytraced_source.h"

using namespace godot;

class ALSourceHandle;

class VAStreamSource : public VARaytracedSource
{
    GDCLASS(VAStreamSource, VARaytracedSource);

private:
    ALStreamBuffer stream_buffer;

    bool stream_open = false;

    // Latched once the emitter is first ready to play, so a raytrace_once stream keeps accepting data after its emitter leaves the world
    bool stream_ready = false;

    // Set when the bounds stopped the sound, or the stream was opened outside them, so it only restarts once inside if the bounds were the reason
    bool stopped_by_bounds = false;

    std::vector<uint8_t> drained_chunk;

    void drain_used_chunks();

    bool start_sound();

    void apply_bounds_rules();

protected:
    static void _bind_methods();

public:
    VAStreamSource();
    ~VAStreamSource();

    void _process(double delta) override;

    void _validate_property(PropertyInfo &p_property) const;

    // If outside the world bounds the stream opens without a sound, then starts once inside
    bool open_stream(int format, int frequency);

    void push_audio_data(const PackedByteArray &data);

    void push_audio_data(const uint8_t *data, int num_bytes);

    void close_stream();

    // Closes the stream too, else pushed data would queue forever with no source playing it
    void stop() override;

    bool is_stream_open() const
    {
        return stream_open;
    }
};
