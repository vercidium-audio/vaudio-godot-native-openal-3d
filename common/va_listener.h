#pragma once

#include "va_emitter.h"

#include <godot_cpp/core/property_info.hpp>

using namespace godot;

namespace va_godot
{

class VAListener : public VAEmitter
{
    GDCLASS(VAListener, VAEmitter);

private:
    bool current = false;

protected:
    static void _bind_methods();

    void attach_to_world() override;
    void detach_from_world() override;

public:
    VAListener();

    bool is_main_listener() const override
    {
        return true;
    }

    void _validate_property(PropertyInfo &p_property) const;

    bool is_current() const;
    void set_current(bool value);
    void make_current();

    // Called by VAWorld. Every listener in a world shares one SDK emitter, which is controlled by the current listener
    void activate(::VAEmitter *shared_handle);
    void deactivate();
    void release_shared_handle();
};

} // namespace va_godot
