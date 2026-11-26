// AI-created

#include "va_source_leech.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_emitter.h"
#include "va_engine_util.h"

void VASourceLeech::_bind_methods()
{
    // Read-only properties (no ADD_PROPERTY)
    ClassDB::bind_method(D_METHOD("get_muffling_gain_lf"), &VASourceLeech::get_muffling_gain_lf);
    ClassDB::bind_method(D_METHOD("get_muffling_gain_hf"), &VASourceLeech::get_muffling_gain_hf);
    ClassDB::bind_method(D_METHOD("is_raytraced"), &VASourceLeech::is_raytraced);
    ClassDB::bind_method(D_METHOD("is_ready_to_play"), &VASourceLeech::is_ready_to_play);
    ClassDB::bind_method(D_METHOD("is_silenced_by_bounds"), &VASourceLeech::is_silenced_by_bounds);
}

VASourceLeech::VASourceLeech() { }

VASourceLeech::~VASourceLeech() { }

float VASourceLeech::get_muffling_gain_lf() const
{
    return get_muffling_gain_lf_value();
}

float VASourceLeech::get_muffling_gain_hf() const
{
    return get_muffling_gain_hf_value();
}

void VASourceLeech::_enter_tree()
{
    if (IS_EDITOR_HINT())
        return;

    emitter = Object::cast_to<va_godot::VAEmitter>(get_parent());

    if (!emitter)
    {
        VA_WARN_NAMED("This source will never play. VASourceLeech nodes must be a direct child of a VAEmitter node.");
        return;
    }

    if (emitter->is_main_listener())
    {
        emitter = nullptr;
        VA_WARN_NAMED("This source will never play. VASourceLeech nodes can't be a child of a VAListener node. Use a VASourceRelative instead.");
        return;
    }
}

void VASourceLeech::_exit_tree()
{
    emitter = nullptr;
}

bool VASourceLeech::is_raytraced() const
{
    return emitter && emitter->is_raytraced();
}

bool VASourceLeech::is_ready_to_play() const
{
    return emitter && emitter->is_ready_to_play();
}

bool VASourceLeech::is_silenced_by_bounds() const
{
    return emitter && emitter->is_silenced_by_bounds();
}

bool VASourceLeech::play(float start_time)
{
    bool silenced = is_silenced_by_bounds();

    // Returns true while waiting for the parent emitter to be ready, and false while silenced by the bounds, even for a loop that will play once inside
    if (!playback.request_play(*this, silenced, is_ready_to_play(), start_time))
        return !silenced;

    bool played = VAPlayer3D::play(start_time);

    if (played)
        playback.on_played();

    return played;
}

void VASourceLeech::stop()
{
    playback.cancel();
    VAPlayer3D::stop();
}

void VASourceLeech::_ready()
{
}

void VASourceLeech::_process(double delta)
{
    VAPlayer3D::_process(delta);

    // Stops the sound while the parent emitter is outside the bounds, and starts a queued play or autoplay once it's ready
    if (playback.process(*this, is_silenced_by_bounds(), is_ready_to_play(), get_autoplay()))
        play(playback.get_pending_start_time());

    if (!is_ready_to_play())
        return;

    // Copies the parent VAEmitter's muffling and reverb, which it updates in its own _process before this child's. The effect is null if it casts no reverb rays
    reverb_effect = emitter->get_reverb_effect();
    update_filter(emitter->get_muffling_gain_lf(), emitter->get_muffling_gain_hf(), true);
}