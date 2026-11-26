// AI-created

#pragma once

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include "backend/va_audio_types.h"
#include "va_raytraced_source.h"

using namespace godot;

class VAStreamSource : public VARaytracedSource
{
    GDCLASS(VAStreamSource, VARaytracedSource);

private:
    // In voices while it plays, nullptr while the bounds have stopped it
    VAStream *stream = nullptr;

    VAPcmFormat stream_format = VAPcmFormat::Mono16;
    int stream_frequency = 0;

    bool stream_open = false;

    // Set once the emitter is first ready to play and stays true, so a raytrace_once stream keeps accepting data after its emitter leaves the world
    bool stream_ready = false;

    // Set when the bounds stopped the sound, or the stream was opened outside them, so it only restarts once inside if the bounds were the reason
    bool stopped_by_bounds = false;

    // The stream's queued audio lives in the backend's stream, so a fresh one starts from the next push
    bool start_sound();

    void stop_sound();

    void apply_bounds_rules();

protected:
    static void _bind_methods();

    void clear_voices() override;

public:
    VAStreamSource();
    ~VAStreamSource();

    void _process(double delta) override;

    void _validate_property(PropertyInfo &p_property) const;

    // format is a VAPcmFormat. If outside the world bounds the stream opens without a sound, then starts once inside
    bool open_stream(int format, int frequency);

    void push_audio_data(const PackedByteArray &data);

    void push_audio_data(const uint8_t *data, int num_bytes);

    void close_stream();

    // Closes the stream too, else pushed data would queue forever with no voice playing it
    void stop() override;

    // Only logs a warning, as a stream only plays the audio pushed to it
    void seek(float time) override;

    bool is_stream_open() const
    {
        return stream_open;
    }
};
