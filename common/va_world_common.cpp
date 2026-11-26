// AI-created

#include "va_world.h"

#include <godot_cpp/variant/callable_method_pointer.hpp>

#include "backend/va_audio_backend.h"
#include "va_conversions.h"
#include "va_custom_material.h"
#include "va_emitter.h"
#include "va_engine_util.h"
#include "va_listener.h"

#include <algorithm>

namespace va_godot
{

// Smallest id reserved for custom (non-built-in) materials - matches vaudio.h's VAMaterialType comment and vaudio-unreal's FirstCustomMaterialId.
static constexpr int FirstCustomMaterialId = 1000;

VAWorld::~VAWorld()
{
    if (world)
    {
        // Will block the main thread if the user hasn't set pendingShutdown=true first.
        vaWorldWait(world);

        // Invokes OnRemoved, which destroys them, for the emitters whose removal was waiting on raytracing results
        VAResult result = vaWorldDestroy(world);

        // Should never fail as we've called vaWorldWait() above.
        if (result != VA_SUCCESS)
            VA_ERROR("Failed to destroy the world (VAResult=", VAResultToString(result), ")");

        world = nullptr;

        // vaWorldDestroy unlinked the emitters still waiting on a reverb tail, so they can be freed now
        VAEmitter::destroy_orphaned_handles(this);
    }
}

bool VAWorld::register_custom_material(va_godot::VACustomMaterial *material)
{
    // Get the lowest id not already claimed by other custom materials.
    int type = FirstCustomMaterialId;

    for (const auto &kvp : custom_materials)
        if (kvp.first >= type)
            type = kvp.first + 1;

    material->set_material_type(type);
    custom_materials[type] = material->get_name();
    return true;
}

void VAWorld::register_emitter(va_godot::VAEmitter *emitter, bool is_main_listener)
{
    VAResult result = vaWorldAddEmitter(world, emitter->get_handle());

    switch (result)
    {
        case VA_SUCCESS:
            break;

        case VA_ALREADY_EXISTS:
            VA_ERROR_NAMED("Failed to register emitter '", emitter->get_name(), "' as it is already added to this VAWorld.");
            break;

        case VA_WORLD_CONFLICT:
            VA_ERROR_NAMED("Failed to register emitter '", emitter->get_name(), "' as it is already added to a different VAWorld.");
            break;

        default:
            VA_ERROR_NAMED_RESULT(result, "Failed to register emitter '", emitter->get_name(), "'.");
            break;
    }

    // Listeners are wired up by set_current_listener
    if (is_main_listener)
        return;

    // Keep track of all emitters
    registered_emitters.push_back(emitter);

    if (listener)
    {
        listener->add_target(emitter);
        return;
    }

    // If this node was added before the VAlistener was created, we need to defer-process all sources/emitters later
    if (!wire_pending_targets_queued)
    {
        wire_pending_targets_queued = true;
        callable_mp(this, &VAWorld::wire_pending_targets).call_deferred();
    }
}

void VAWorld::wire_pending_targets()
{
    wire_pending_targets_queued = false;

    if (!listener)
        return;

    for (va_godot::VAEmitter *emitter : registered_emitters)
    {
        // Skip a source whose SDK handle has already been torn down (raytrace_once removal, pending destroy) but whose node is still briefly in registered_emitters.
        if (emitter->get_handle())
            listener->add_target(emitter);
    }
}

void VAWorld::unregister_pending_target(va_godot::VAEmitter *emitter)
{
    registered_emitters.erase(std::remove(registered_emitters.begin(), registered_emitters.end(), emitter), registered_emitters.end());
}

void VAWorld::register_listener(va_godot::VAListener *node)
{
    listeners.push_back(node);

    if (!listener)
    {
        set_current_listener(node);
        return;
    }

    // A listener added with current=true takes over, e.g. a player scene that was spawned with its VAListener's current property enabled
    if (node->is_current())
    {
        VA_WARN_NAMED("VAListener '", node->get_name(), "' has current enabled, so it replaced '", listener->get_name(), "' as the current listener. Disable current on listeners that shouldn't take over when added, and call make_current() on the one that should be used.");
        set_current_listener(node);
    }
}

void VAWorld::unregister_listener(va_godot::VAListener *node)
{
    listeners.erase(std::remove(listeners.begin(), listeners.end(), node), listeners.end());

    if (listener != node)
        return;

    if (!listeners.empty())
    {
        set_current_listener(listeners.front());
        return;
    }

    // Last listener in this world, so the shared handle goes with it
    node->release_shared_handle();
    listener = nullptr;

    // A listener may come back (e.g. scene reload), so let a future missing-listener state warn again.
    warned_missing_listener = false;
}

void VAWorld::set_current_listener(va_godot::VAListener *node)
{
    if (listener == node)
        return;

    va_godot::VAListener *previous = static_cast<va_godot::VAListener *>(listener);
    ::VAEmitter *shared_handle = previous ? previous->get_handle() : nullptr;

    if (previous)
        previous->deactivate();

    listener = node;
    node->activate(shared_handle);

    // Set up the sources that were created before the listener existed
    if (!shared_handle)
        wire_pending_targets();
}

void VAWorld::release_current_listener(va_godot::VAListener *node)
{
    if (listener != node)
    {
        node->deactivate();
        return;
    }

    for (va_godot::VAListener *other : listeners)
    {
        if (other != node)
        {
            set_current_listener(other);
            return;
        }
    }

    VA_WARN_NAMED("VAListener '", node->get_name(), "' is the only listener in this world, so it stays current.");
}

void VAWorld::on_reverb_updated_trampoline(::VAWorld *world)
{
    VAWorld *self = static_cast<VAWorld *>(vaWorldGetUserData(world));

    if (self)
    {
        self->on_reverb_updated();
        self->raytrace_count++;
    }
}

void VAWorld::on_reverb_updated()
{
    if (!listener || !listener->get_handle())
    {
        // Don't warn during teardown - a reverb update can still be in flight after the listener node has unregistered but before this VAWorld node is destroyed.
        if (!warned_missing_listener && !is_shutting_down)
        {
            VA_WARN_NAMED("Has no VAListener node, so reverb cannot be updated. ", MISSING_LISTENER_HINT);
            warned_missing_listener = true;
        }

        return;
    }

    VAEAXReverb *eax = vaEmitterGetEAX(listener->get_handle());

    if (eax && listener_reverb_effect)
        listener_reverb_effect->set_params(*eax, 1.0f, Vector3(), Vector3());

    VAAudioBackend *manager = VAAudioBackend::get_singleton();
    int grouped_eax_count = vaWorldGetGroupedEAXCount(world);
    const VAEAXReverb **grouped_eax = vaWorldGetGroupedEAX(world);

    for (int i = 0; i < grouped_eax_count; i++)
    {
        if ((int)grouped_reverb_effects.size() <= i)
        {
            std::unique_ptr<VAReverbEffect> new_effect = manager ? manager->create_reverb_effect() : nullptr;

            if (!new_effect)
                break;

            GroupedReverbEffect grouped;
            grouped.effect = std::move(new_effect);
            grouped_reverb_effects.push_back(std::move(grouped));
        }

        GroupedReverbEffect &grouped = grouped_reverb_effects[i];

        // A grouped zone the listener hasn't raytraced yet keeps its last gain and pan - vaEAXReverbGetRelative* return NULL for "no entry yet"
        float *relative_gain = vaEAXReverbGetRelativeGain(grouped_eax[i], listener->get_handle());
        VAVector *relative_direction = vaEAXReverbGetRelativeDirection(grouped_eax[i], listener->get_handle());

        if (relative_gain)
            grouped.gain = std::clamp(*relative_gain, 0.0f, 1.0f);

        if (relative_direction)
            grouped.pan = get_grouped_eax_pan(*relative_direction);

        // TODO - separate pan for late reverb and reflections
        grouped.effect->set_params(*grouped_eax[i], grouped.gain, grouped.pan, grouped.pan);
    }
}

VAReverbEffect *VAWorld::get_reverb_effect(::VAEmitter *emitter)
{
    VAEmitter *self = emitter ? static_cast<VAEmitter *>(vaEmitterGetUserData(emitter)) : nullptr;

    // Not raytraced outside the bounds, so any grouped EAX it has is stale, and the listener's reverb doesn't describe where it is either
    if (self && self->is_outside_bounds())
        return nullptr;

    if (emitter && vaEmitterGetAffectsGroupedEAX(emitter))
    {
        int grouped_eax_index = vaEmitterGetGroupedEAXIndex(emitter);

        if (grouped_eax_index >= 0)
        {
            if (grouped_eax_index >= (int)grouped_reverb_effects.size())
            {
                // Grouped reverb effects are only created once reverb updates with a listener
                if (!listener || !listener->get_handle())
                    VA_WARN("Emitter has a grouped EAX index of ", grouped_eax_index, " but the VAWorld has no VAListener, so no EAX presets are available. ", MISSING_LISTENER_HINT);
                else
                    VA_WARN(
                        "Emitter has a grouped EAX index of ", grouped_eax_index,
                        " but only ", (int)grouped_reverb_effects.size(), " EAX presets are available. Increase the 'Maximum Grouped EAX Count' setting on the VAWorld node.");
                return listener_reverb_effect.get();
            }

            return grouped_reverb_effects[grouped_eax_index].effect.get();
        }
    }

    if (self && !self->get_use_listener_reverb())
        return nullptr;

    return listener_reverb_effect.get();
}

bool VAWorld::is_listener_reverb_ready() const
{
    if (!listener || !listener->get_handle())
        return false;

    return !listener_reverb_effect || !listener_reverb_effect->is_valid() || !listener->is_waiting_for_eax() || listener->is_outside_bounds();
}

void VAWorld::warn_if_listener_casts_no_reverb_rays(const Node *source, bool &warned) const
{
    if (warned || !listener || !listener->get_handle() || vaEmitterGetReverbEnabled(listener->get_handle()))
        return;

    warned = true;
    VA_WARN(source->get_name(), ": The listener '", listener->get_name(), "' casts no reverb rays, so this source plays without the listener's reverb. Set the listener's reverb ray and bounce counts above 0.");
}

} // namespace va_godot
