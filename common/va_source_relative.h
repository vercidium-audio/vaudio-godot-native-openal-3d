// Human-reviewed

#pragma once

#include "al_source_relative.h"

using namespace godot;

// Alias the VAWorld node, not the C SDK struct
namespace va_godot { class VAWorld; }

class VASourceRelative : public ALSourceRelative
{
    GDCLASS(VASourceRelative, ALSourceRelative);

private:
    va_godot::VAWorld *va_world = nullptr;

    // Set to true if _enter_tree did not find a VAWorld
    bool waiting_for_world = false;

    void retry_find_va_world(Node *node);

protected:
    static void _bind_methods();

public:
    VASourceRelative();
    ~VASourceRelative();

    void _enter_tree() override;
    void _exit_tree() override;

    bool play(float start_time = 0.0f) override;
};
