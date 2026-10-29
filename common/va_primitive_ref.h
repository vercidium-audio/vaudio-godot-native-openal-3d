#pragma once

#include <functional>

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>

extern "C"
{
#include <vaudio.h>
}

#include "transform_watcher.h"
#include "va_primitive_kind.h"

using namespace godot;

// Stashed as Node metadata (see VAWorld::PRIMITIVE_META_KEY) so a primitive can be removed/updated later when its owning node moves or leaves the scene tree.
class VAPrimitiveRef : public RefCounted
{
    GDCLASS(VAPrimitiveRef, RefCounted);

protected:
    static void _bind_methods()
    {
    }

public:
    void *primitive = nullptr;
    VAPrimitiveKind kind; // always set explicitly at construction, no sensible dimension-neutral default
    TransformWatcher *watcher = nullptr;

    // Only set for CollisionShape nodes - the Shape resource whose changed signal is connected to shape_callable, so remove_primitive can disconnect it
    Ref<Resource> shape_resource;
    Callable shape_callable;

    // shape_callable targets this ref rather than VAWorld, because Godot treats bound callables that only differ in their bound arguments as equal, so CollisionShapes sharing one Shape resource would collide on connect and disconnect
    std::function<void()> on_shape_changed;

    void shape_changed()
    {
        // The callback can recreate the primitive and drop the last reference to this ref, so keep it and the callback alive until it returns
        Ref<VAPrimitiveRef> keep_alive(this);
        std::function<void()> callback = on_shape_changed;

        if (callback)
            callback();
    }

    // The VAWorld::SharedMesh (3D) or VAWorld::SharedPolygon (2D) this primitive was created from, released in remove_primitive
    void *shared_geometry = nullptr;
};
