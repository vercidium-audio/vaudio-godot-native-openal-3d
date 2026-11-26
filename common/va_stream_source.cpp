// AI-created

#include "va_stream_source.h"

#include <godot_cpp/core/class_db.hpp>

#include "backend/va_audio_backend.h"
#include "va_engine_util.h"

#include <algorithm>

VAStreamSource::VAStreamSource()
{
}

VAStreamSource::~VAStreamSource()
{
    close_stream();
}

void VAStreamSource::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("open_stream", "format", "frequency"), &VAStreamSource::open_stream);
    ClassDB::bind_method(D_METHOD("push_audio_data", "data"), static_cast<void (VAStreamSource::*)(const PackedByteArray &)>(&VAStreamSource::push_audio_data));
    ClassDB::bind_method(D_METHOD("close_stream"), &VAStreamSource::close_stream);
    ClassDB::bind_method(D_METHOD("is_stream_open"), &VAStreamSource::is_stream_open);
}

bool VAStreamSource::open_stream(int format, int frequency)
{
    close_stream();

    if (format < (int)VAPcmFormat::Mono8 || format > (int)VAPcmFormat::StereoFloat)
    {
        VA_ERROR_NAMED(format, " isn't a VAPcmFormat");
        return false;
    }

    stream_format = (VAPcmFormat)format;
    stream_frequency = frequency;
    stopped_by_bounds = is_silenced_by_bounds();

    if (!stopped_by_bounds && !start_sound())
        return false;

    stream_open = true;
    stream_ready = false;
    return true;
}

bool VAStreamSource::start_sound()
{
    VAAudioBackend *manager = VAAudioBackend::get_singleton();

    // The backend already logged the reason for failure
    std::unique_ptr<VAStream> new_stream = manager ? manager->create_stream(stream_format, stream_frequency) : nullptr;

    if (!new_stream)
        return false;

    VAVoice &voice = *new_stream->get_voice();
    voice.set_gain(get_gain());
    voice.set_pitch(get_pitch());

    configure_voice(voice);

    update_dry_output();
    apply_filters(voice);

    // The stream's voice starts playing as soon as it's created. The is_ready_to_play() check happens below in push_audio_data()
    apply_paused(voice);

    stream = new_stream.get();

    VAPlayerVoice entry;
    entry.stream = std::move(new_stream);
    voices.push_back(std::move(entry));
    return true;
}

void VAStreamSource::stop_sound()
{
    for (auto it = voices.begin(); it != voices.end(); ++it)
    {
        if (it->stream.get() == stream)
        {
            voices.erase(it);
            break;
        }
    }

    stream = nullptr;
}

void VAStreamSource::clear_voices()
{
    stream = nullptr;
    VAPlayer::clear_voices();
}

void VAStreamSource::seek(float time)
{
    VA_WARN_NAMED("A VAStreamSource can't seek, as it only plays the audio pushed to it");
}

void VAStreamSource::apply_bounds_rules()
{
    if (!stream_open)
        return;

    if (is_silenced_by_bounds())
    {
        if (!stream)
            return;

        stop_sound();
        stopped_by_bounds = true;
        return;
    }

    if (!stopped_by_bounds)
        return;

    // Audio queued before it left would play late, so the new sound starts from the next push
    stopped_by_bounds = false;

    if (!start_sound())
    {
        VA_WARN_NAMED("The stream failed to create its sound, so it was closed");
        close_stream();
    }
}

void VAStreamSource::push_audio_data(const PackedByteArray &data)
{
    push_audio_data(data.ptr(), (int)data.size());
}

void VAStreamSource::push_audio_data(const uint8_t *data, int num_bytes)
{
    if (!stream_open)
    {
        VA_ERROR_NAMED("push_audio_data() failed as the stream never opened");
        return;
    }

    // Nothing plays it while the bounds have stopped it, and audio queued while paused would play late
    if (num_bytes == 0 || stopped_by_bounds || get_stream_paused() || !stream)
        return;

    // Ignore data that arrives before raytracing completed, so the stream never plays unmuffled or without reverb
    if (!stream_ready)
    {
        if (!is_ready_to_play())
            return;

        stream_ready = true;
    }

    stream->enqueue(data, num_bytes);
}

void VAStreamSource::close_stream()
{
    if (!stream_open)
        return;

    stop_sound();
    stream_open = false;
    stopped_by_bounds = false;
}

void VAStreamSource::stop()
{
    close_stream();
    VAPlayer::stop();
}

void VAStreamSource::_validate_property(PropertyInfo &p_property) const
{
    static const StringName hidden_properties[] = {
        "streams",
        "looping",
        "autoplay",
        "pitch_randomness",
        "volume_randomness_db",
        "playback_no_repeat",
    };

    for (const StringName &hidden_property : hidden_properties)
    {
        if (p_property.name == hidden_property)
        {
            p_property.usage = PROPERTY_USAGE_NONE;
            return;
        }
    }
}

void VAStreamSource::_process(double delta)
{
    VARaytracedSource::_process(delta);

    // VAPlayer::_process drops a finished voice, which would take the stream with it
    if (stream && std::none_of(voices.begin(), voices.end(), [this](const VAPlayerVoice &voice) { return voice.stream.get() == stream; }))
        stream = nullptr;

    process_raytracing(delta);
    apply_bounds_rules();

    // Also set stream_ready here in case nothing is pushed this frame. A raytrace_once emitter removes itself from the world later this frame, after which is_ready_to_play() is false and push_audio_data() would drop everything
    if (stream_open && !stream_ready && is_ready_to_play())
    {
        stream_ready = true;
    }
}
