// Human-created

#include "va_source.h"

#include <godot_cpp/core/class_db.hpp>

void VASource::_bind_methods()
{
}

VASource::VASource()
{
}

VASource::~VASource()
{
}

void VASource::_ready()
{
}

bool VASource::play(float start_time)
{
    bool silenced = is_silenced_by_bounds();

    // Returns true while waiting for the emitter to be ready, and false while silenced by the bounds, even for a loop that will play once inside
    if (!playback.request_play(*this, silenced, is_ready_to_play(), start_time))
        return !silenced;

    bool played = VARaytracedSource::play(start_time);

    if (played)
        playback.on_played();

    return played;
}

void VASource::stop()
{
    playback.cancel();
    VARaytracedSource::stop();
}

void VASource::_process(double delta)
{
    VARaytracedSource::_process(delta);
    process_raytracing(delta);

    if (playback.process(*this, is_silenced_by_bounds(), is_ready_to_play(), get_autoplay()))
        play(playback.get_pending_start_time());
}
