// Human-created

#include "va_material_inspector_plugin.h"

#include "al_source.h"
#include "va_emitter.h"
#include "va_world.h"

using namespace va_godot;

bool VAMaterialInspectorPlugin::_can_handle(Object *object) const
{
    // VAWorld/VAEmitter/VAListener/ALSource/VASource* are 3D nodes but don't have geometry
    if (Object::cast_to<VAWorld>(object) || Object::cast_to<VAEmitter>(object) || Object::cast_to<ALSource>(object))
        return false;

    return Object::cast_to<Node3D>(object) != nullptr;
}
