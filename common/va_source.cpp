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

bool VASource::play()
{
    // Wait for the muffling and reverb results, so the sound never starts unmuffled or without reverb
    if (!is_ready_to_play())
    {
        return false;
    }

    played = VARaytracedSource::play();

    return played;
}

void VASource::_process(double delta)
{
    VARaytracedSource::_process(delta);
    process_raytracing(delta);

    if (!played && get_autoplay() && is_ready_to_play())
    {
        play();
    }
}
