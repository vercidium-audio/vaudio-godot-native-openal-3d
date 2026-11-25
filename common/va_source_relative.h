// Human-reviewed

#pragma once

#include "al_source_relative.h"
#include "va_bounds_playback.h"

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

    bool warned_no_listener_reverb = false;

    void retry_find_va_world(Node *node);

    // Never silenced by the bounds, as it plays at the listener
    VABoundsPlayback playback;

protected:
    static void _bind_methods();

public:
    VASourceRelative();
    ~VASourceRelative();

    void _enter_tree() override;
    void _exit_tree() override;

    // Autoplay waits for _process, once the source is ready to play
    void _ready() override;

    void _process(double delta) override;

    // True once the current listener's reverb has its first raytraced results, or never will (no reverb rays, or outside the bounds). Sources don't play until this is true, so they never start in OpenAL's default reverb
    bool is_ready_to_play() const;

    // Waits until the source is ready to play, and returns true while waiting
    bool play(float start_time = 0.0f) override;

    // Also cancels a play or autoplay that's waiting
    void stop() override;
};
