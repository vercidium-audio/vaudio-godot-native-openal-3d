#include "va_bounds_playback.h"

#include <godot_cpp/variant/utility_functions.hpp>

#include "al_source.h"
#include "va_engine_util.h"

bool VABoundsPlayback::stop_if_silenced(ALSource &source, bool silenced)
{
    if (!silenced)
        return false;

    if (!source.get_looping())
        cancel();
    // A paused loop is re-queued too, and starts paused once inside. It restarts from the beginning, unless a play from a start time is already queued
    else if (source.has_sound() || source.is_play_requested())
    {
        if (!play_pending)
            pending_start_time = 0.0f;

        play_pending = true;
    }

    // The base stop(), since the overrides cancel the queued play
    source.ALSource::stop();
    return true;
}

bool VABoundsPlayback::request_play(ALSource &source, bool silenced, bool ready, float start_time)
{
    if (silenced && !source.get_looping())
        VA_WARN(source.get_name(), ": This source did not play because it is outside the world bounds.");

    if (!silenced && ready)
        return true;

    // Waits for the emitter's muffling and reverb results, so the sound never starts unmuffled or without reverb. stop_if_silenced() cancels it if it doesn't loop and is outside the bounds
    play_pending = true;
    pending_start_time = start_time;
    stop_if_silenced(source, silenced);
    return false;
}

bool VABoundsPlayback::process(ALSource &source, bool silenced, bool ready, bool autoplay)
{
    if (stop_if_silenced(source, silenced))
        return false;

    return ready && (play_pending || (autoplay && !autoplay_done));
}
