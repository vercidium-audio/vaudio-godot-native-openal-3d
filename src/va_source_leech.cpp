// Human-reviewed

#include "va_source_leech.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_emitter.h"
#include "va_engine_util.h"

void VASourceLeech::_bind_methods()
{
    // Read-only muffling stats - no ADD_PROPERTY, same rationale as VASource's.
    ClassDB::bind_method(D_METHOD("get_muffling_gain_lf"), &VASourceLeech::get_muffling_gain_lf);
    ClassDB::bind_method(D_METHOD("get_muffling_gain_hf"), &VASourceLeech::get_muffling_gain_hf);
    ClassDB::bind_method(D_METHOD("is_raytraced"), &VASourceLeech::is_raytraced);
    ClassDB::bind_method(D_METHOD("is_ready_to_play"), &VASourceLeech::is_ready_to_play);
    ClassDB::bind_method(D_METHOD("is_silenced_by_bounds"), &VASourceLeech::is_silenced_by_bounds);
}

VASourceLeech::VASourceLeech()
{
}

VASourceLeech::~VASourceLeech()
{
}

float VASourceLeech::get_muffling_gain_lf() const
{
    return filter.get_gain();
}

float VASourceLeech::get_muffling_gain_hf() const
{
    return filter.get_gain_hf();
}

void VASourceLeech::_enter_tree()
{
    if (IS_EDITOR_HINT())
    {
        return;
    }

    emitter = Object::cast_to<va_godot::VAEmitter>(get_parent());

    if (!emitter)
    {
        VA_WARN_NAMED("This source will never play. VASourceLeech nodes must be a direct child of a VAEmitter node.");
        return;
    }

    if (emitter->is_main_listener())
    {
        emitter = nullptr;
        VA_WARN_NAMED("This source will never play. VASourceLeech nodes can't be a child of a VAListener, use a VASourceRelative instead.");
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

bool VASourceLeech::play()
{
    if (playback.apply_on_play(*this, is_silenced_by_bounds()))
        return false;

    // Must wait for the parent emitter to raytrace at least once
    if (!is_ready_to_play())
        return false;

    bool played = ALSource3D::play();

    if (played)
        playback.on_played();

    return played;
}

void VASourceLeech::stop()
{
    playback.cancel();
    ALSource3D::stop();
}

void VASourceLeech::_ready()
{
}

void VASourceLeech::_process(double delta)
{
    ALSource3D::_process(delta);

    if (!is_raytraced())
        return;

    bool silenced = playback.apply(*this, is_silenced_by_bounds());

    if (!silenced && playback.is_pending(get_autoplay()) && is_ready_to_play())
        play();

    apply_raytracing_results();
}

// Copies the parent VAEmitter's muffling and reverb, which it updates in its own _process before this child's
void VASourceLeech::apply_raytracing_results()
{
    effect = emitter->get_effect();

    if (emitter->is_ready_to_play())
        update_filter(emitter->get_muffling_gain_lf(), emitter->get_muffling_gain_hf(), true);
}
