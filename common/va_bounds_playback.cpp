#include "va_bounds_playback.h"

#include <godot_cpp/variant/utility_functions.hpp>

#include "al_source.h"
#include "va_engine_util.h"

bool VABoundsPlayback::apply(ALSource &source, bool silenced)
{
    if (!silenced)
        return false;

    if (!source.get_looping())
        cancel();
    else if (source.is_playing() || source.is_play_requested())
        play_pending = true;

    // The base stop(), since the overrides cancel the pending play
    source.ALSource::stop();
    return true;
}

bool VABoundsPlayback::apply_on_play(ALSource &source, bool silenced)
{
    if (!silenced)
        return false;

    if (!source.get_looping())
        VA_WARN(source.get_name(), ": play() was called outside the VAWorld's bounds, so nothing played. A sound that doesn't loop isn't started outside them, and doesn't play once back inside");

    play_pending = true;
    return apply(source, silenced);
}
