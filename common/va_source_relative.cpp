#include "va_source_relative.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_emitter.h"
#include "va_engine_util.h"
#include "va_world.h"
#include "va_world_lookup.h"

void VASourceRelative::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("is_ready_to_play"), &VASourceRelative::is_ready_to_play);
}

VASourceRelative::VASourceRelative()
{
}

VASourceRelative::~VASourceRelative()
{
}

void VASourceRelative::_enter_tree()
{
    if (IS_EDITOR_HINT())
    {
        return;
    }

    va_world = va_godot::find_va_world(this);

    if (!va_world)
    {
        // No VAWorld anywhere in the tree yet - stay dormant and retry on every future node addition (see VAEmitter::_enter_tree).
        waiting_for_world = true;
        get_tree()->connect("node_added", callable_mp(this, &VASourceRelative::retry_find_va_world));
    }
}

void VASourceRelative::_exit_tree()
{
    if (waiting_for_world)
    {
        if (get_tree() && get_tree()->is_connected("node_added", callable_mp(this, &VASourceRelative::retry_find_va_world)))
        {
            get_tree()->disconnect("node_added", callable_mp(this, &VASourceRelative::retry_find_va_world));
        }

        waiting_for_world = false;

        // Never found a VAWorld anywhere in the tree for this node's entire time in it (see VAEmitter::_exit_tree).
        VA_WARN(
            "'", get_name(),
            "' left the tree without ever finding a VAWorld - "
            "no emitter was created for it. Make sure this node's scene "
            "was added under a VAWorld while it was in the tree.");
    }
}

// Re-attempts find_va_world each time a node is added anywhere in the tree (see VAEmitter::retry_find_va_world).
void VASourceRelative::retry_find_va_world(Node *node)
{
    va_world = va_godot::find_va_world(this);

    if (!va_world)
    {
        return;
    }

    get_tree()->disconnect("node_added", callable_mp(this, &VASourceRelative::retry_find_va_world));
    waiting_for_world = false;
}

void VASourceRelative::_ready()
{
}

void VASourceRelative::_process(double delta)
{
    ALSourceRelative::_process(delta);

    if (!IS_EDITOR_HINT() && playback.process(*this, false, is_ready_to_play(), get_autoplay()))
        play(playback.get_pending_start_time());
}

bool VASourceRelative::is_ready_to_play() const
{
    return va_world && va_world->is_listener_reverb_ready();
}

bool VASourceRelative::play(float start_time)
{
    if (IS_EDITOR_HINT())
    {
        return false;
    }

    if (!playback.request_play(*this, false, is_ready_to_play(), start_time))
        return true;

    // Matches VASourceRelative.cs: relative sources aren't raytraced and have no emitter of their own, so they always share
    // the listener's room reverb rather than any grouped-EAX slot (nullptr short-circuits get_reverb_effect to the listener slot).
    effect = va_world->get_reverb_effect(nullptr);

    update_filter(1.0f, 1.0f);

    bool played = ALSourceRelative::play(start_time);

    if (played)
        playback.on_played();

    return played;
}

void VASourceRelative::stop()
{
    playback.cancel();
    ALSourceRelative::stop();
}
