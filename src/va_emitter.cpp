#include "va_emitter.h"

#include "va_visualisation.h"

// The rest of VAEmitter is dimension-agnostic and lives in common/va_emitter.cpp.

namespace va_godot
{

Vector3 VAEmitter::get_va_position() const
{
    VAVector position = vaEmitterGetPosition(get_handle());
    return Vector3(position.x, position.y, position.z);
}

void VAEmitter::initialize_visualisation()
{
    if (visualisation)
        visualisation->initialize_visualisation();
}

void VAEmitter::teardown_visualisation()
{
    if (visualisation)
        visualisation->teardown_visualisation();
}

} // namespace va_godot
