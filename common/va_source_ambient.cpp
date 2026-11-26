// AI-created

#include "va_source_ambient.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_emitter.h"
#include "va_engine_util.h"
#include "va_world.h"
#include "va_world_lookup.h"

void VASourceAmbient::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_use_listener_reverb"), &VASourceAmbient::get_use_listener_reverb);
    ClassDB::bind_method(D_METHOD("set_use_listener_reverb", "value"), &VASourceAmbient::set_use_listener_reverb);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_listener_reverb"), "set_use_listener_reverb", "get_use_listener_reverb");
}

VASourceAmbient::VASourceAmbient()
{
}

VASourceAmbient::~VASourceAmbient()
{
}

void VASourceAmbient::_enter_tree()
{
    if (IS_EDITOR_HINT())
        return;

    va_world = va_godot::find_va_world(this);

    if (!va_world)
    {
        // No VAWorld anywhere in the tree yet - stay dormant and retry on every future node addition (see VAEmitter::_enter_tree).
        waiting_for_world = true;
        get_tree()->connect("node_added", callable_mp(this, &VASourceAmbient::retry_find_va_world));
    }
}

void VASourceAmbient::_exit_tree()
{
    if (waiting_for_world)
    {
        SceneTree* tree = get_tree();

        // Unregister callbacks
        if (tree && tree->is_connected("node_added", callable_mp(this, &VASourceAmbient::retry_find_va_world)))
            tree->disconnect("node_added", callable_mp(this, &VASourceAmbient::retry_find_va_world));

        // Never found a VAWorld anywhere in the tree for this node's entire lifetime
        VA_WARN_NAMED("This VASourceAmbient node left the tree without ever finding a VAWorld.");

        waiting_for_world = false;
    }
}

// Re-attempts find_va_world each time a node is added anywhere in the tree (see VAEmitter::retry_find_va_world).
void VASourceAmbient::retry_find_va_world(Node *node)
{
    va_world = va_godot::find_va_world(this);

    if (va_world)
    {
        // Successfully found a world - unregister the callback
        get_tree()->disconnect("node_added", callable_mp(this, &VASourceAmbient::retry_find_va_world));
        waiting_for_world = false;
    }
}

void VASourceAmbient::_process(double delta)
{
    VAPlayer::_process(delta);

    if (IS_EDITOR_HINT())
        return;

    va_godot::VAEmitter *listener = va_world ? va_world->get_listener() : nullptr;

    if (!listener)
        return;

    if (!warned_no_ambience && listener->is_raytraced() && !listener->is_ambient_occlusion_enabled() && !listener->is_ambient_permeation_enabled())
    {
        VA_WARN_NAMED("The listener '", listener->get_name(), "' does not have ambience enabled, so this VASourceAmbient will never play. Set the listener's ambient occlusion / permeation ray and bounce counts above 0.");
        warned_no_ambience = true;
    }

    if (!listener->is_ambient_filter_ready())
        return;

    // nullptr gets the listener's reverb effect. The reverb send is muffled by the ambient filter too
    reverb_effect = use_listener_reverb ? va_world->get_reverb_effect(nullptr) : nullptr;
    update_filter(listener->get_ambient_filter_gain_lf(), listener->get_ambient_filter_gain_hf());

    // Waits for the listener's first EAX, so it never starts in the backend's default reverb
    if (!played && (!use_listener_reverb || va_world->is_listener_reverb_ready()))
    {
        if (use_listener_reverb)
            va_world->warn_if_listener_casts_no_reverb_rays(this, warned_no_listener_reverb);

        played = VAPlayer::play();
    }
}
