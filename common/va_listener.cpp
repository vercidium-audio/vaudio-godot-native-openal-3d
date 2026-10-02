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

// Hides inherited properties not meaningful on a listener: affects_grouped_eax/occlusion_energy_cap/permeation_energy_cap only apply to emitters that can be occluded/permeated, and raytrace_once only applies to emitters that get raytraced against the listener, not the listener itself.
void VAListener::_validate_property(PropertyInfo &p_property) const
{
    if (p_property.name == StringName("has_relative_reverb") ||
        p_property.name == StringName("affects_grouped_eax") ||
        p_property.name == StringName("keep_reverb_tail_alive") ||
        p_property.name == StringName("occlusion_energy_cap") ||
        p_property.name == StringName("permeation_energy_cap") ||
        p_property.name == StringName("raytrace_once"))
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
    // Not in a world yet (e.g. set while loading the scene, or in the editor) - VAWorld::register_listener reads the flag once this node is attached
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

// Takes over the shared handle from the previous listener, or creates it if this is the first listener in the world. Targets stay connected to this listener.
void VAListener::activate(::VAEmitter *shared_handle)
{
    current = true;

    if (!shared_handle)
    {
        create_emitter();
        return;
    }

    emitter = shared_handle;
    vaEmitterSetUserData(emitter, this);
    vaEmitterSetName(emitter, String(get_name()).utf8().get_data());
    vaEmitterSetPosition(emitter, ToVAudio(get_global_position()));

    apply_properties_to_handle();
}

void VAListener::deactivate()
{
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
