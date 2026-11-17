// Human-created

#include "va_listener.h"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "va_conversions.h"
#include "va_world.h"

namespace va_godot
{

void VAListener::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("is_current"), &VAListener::is_current);
    ClassDB::bind_method(D_METHOD("set_current", "value"), &VAListener::set_current);
    ClassDB::bind_method(D_METHOD("make_current"), &VAListener::make_current);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "current"), "set_current", "is_current");
}

VAListener::VAListener()
{
    set_affects_grouped_eax(false);
    set_has_relative_reverb(true);
}

// Hide properties that aren't relevant to the listener
void VAListener::_validate_property(PropertyInfo &p_property) const
{
    if (p_property.name == StringName("has_relative_reverb") ||
        p_property.name == StringName("affects_grouped_eax") ||
        p_property.name == StringName("keep_reverb_tail_alive") ||
        p_property.name == StringName("occlusion_energy_cap") ||
        p_property.name == StringName("permeation_energy_cap") ||
        p_property.name == StringName("raytrace_once") ||
        p_property.name == StringName("clustering_enabled") ||
        p_property.name == StringName("clustering_distance"))
    {
        p_property.usage = PROPERTY_USAGE_NONE;
    }
}

bool VAListener::is_current() const
{
    return current;
}

void VAListener::set_current(bool value)
{
    // Not in a world yet (e.g. set while loading the scene, or in the editor). Will become current in attach_to_world() instead
    if (!va_world)
    {
        current = value;
        return;
    }

    if (value)
        va_world->set_current_listener(this);
    else
        va_world->release_current_listener(this);
}

void VAListener::make_current()
{
    set_current(true);
}

void VAListener::attach_to_world()
{
    va_world->register_listener(this);
}

void VAListener::detach_from_world()
{
    va_world->unregister_listener(this);
}

void VAListener::activate(::VAEmitter *shared_handle)
{
    current = true;

    if (!shared_handle)
    {
        // First listener, create the VAEmitter
        create_emitter();
        return;
    }

    // Take over the shared Emitter from the previous listener
    emitter = shared_handle;
    vaEmitterSetUserData(emitter, this);
    vaEmitterSetName(emitter, String(get_name()).utf8().get_data());
    vaEmitterSetPosition(emitter, ToVAudio(get_global_position()));

    apply_properties_to_handle();

    initialize_visualisation();
}

void VAListener::deactivate()
{
    // Remove all visualisation callbacks / ray counts, so the new current listener can take over
    teardown_visualisation();

    current = false;
    emitter = nullptr;
}

// Only called when the last listener leaves the world
void VAListener::release_shared_handle()
{
    current = false;

    if (emitter)
        release_emitter();
}

} // namespace va_godot
