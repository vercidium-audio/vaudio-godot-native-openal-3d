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

bool VASource::play()
{
    if (playback.apply_on_play(*this, is_silenced_by_bounds()))
        return false;

    // Wait for the muffling and reverb results, so the sound never starts unmuffled or without reverb
    if (!is_ready_to_play())
        return false;

    bool played = VARaytracedSource::play();

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

    if (!playback.apply(*this, is_silenced_by_bounds()) && playback.is_pending(get_autoplay()) && is_ready_to_play())
        play();
}
