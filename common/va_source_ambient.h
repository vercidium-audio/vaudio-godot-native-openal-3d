// Human-created

#pragma once

#include "al_source_relative.h"

using namespace godot;

namespace va_godot
{
class VAWorld;
}

class VASourceAmbient : public ALSourceRelative
{
    GDCLASS(VASourceAmbient, ALSourceRelative);

private:
    va_godot::VAWorld *va_world = nullptr;

    bool played = false;
    bool warned_no_ambience = false;
    bool warned_no_listener_reverb = false;
    bool waiting_for_world = false;
    bool use_listener_reverb = false;

    void retry_find_va_world(Node *node);

protected:
    static void _bind_methods();

public:
    VASourceAmbient();
    ~VASourceAmbient();

    void _enter_tree() override;
    void _exit_tree() override;
    void _process(double delta) override;

    // If true, this source uses the listener's reverb effect, muffled by the ambient filter. It plays when the listener's rays have been cast
    bool get_use_listener_reverb() const { return use_listener_reverb; }
    void set_use_listener_reverb(bool value) { use_listener_reverb = value; }
};
