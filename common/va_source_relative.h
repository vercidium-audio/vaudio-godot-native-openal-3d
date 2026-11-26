// AI-created

#pragma once

#include "va_player.h"
#include "va_bounds_playback.h"

using namespace godot;

// Alias the VAWorld node, not the C SDK struct
namespace va_godot { class VAWorld; }

// Sounds that play at the listener, e.g. footsteps, music, UI
class VASourceRelative : public VAPlayer
{
    GDCLASS(VASourceRelative, VAPlayer);

private:
    va_godot::VAWorld *va_world = nullptr;

    bool use_listener_reverb = true;

    // Set to true if _enter_tree did not find a VAWorld
    bool waiting_for_world = false;

    bool warned_no_listener_reverb = false;

    void retry_find_va_world(Node *node);

    // Never silenced by the bounds, as it plays at the listener
    VABoundsPlayback playback;

protected:
    static void _bind_methods();

    void configure_voice(VAVoice &voice) override
    {
        configure_listener_relative_voice(voice);
    }

public:
    VASourceRelative();
    ~VASourceRelative();

    void _enter_tree() override;
    void _exit_tree() override;

    // Autoplay waits for _process, once the source is ready to play
    void _ready() override;

    void _process(double delta) override;

    // If true, this source is sent to the listener's reverb effect, and waits for the listener's first raytraced reverb before it plays. If false, it plays straight away with no reverb (e.g. music, UI), and needs no VAWorld
    bool get_use_listener_reverb() const
    {
        return use_listener_reverb;
    }

    void set_use_listener_reverb(bool value)
    {
        use_listener_reverb = value;
    }

    // True once the current listener's reverb has its first raytraced results, or never will (no reverb rays, or outside the bounds). Sources don't play until this is true, so they never start in the backend's default reverb. Always true when use_listener_reverb is off
    bool is_ready_to_play() const;

    // Waits until the source is ready to play, and returns true while waiting
    bool play(float start_time = 0.0f) override;

    // Also cancels a play or autoplay that's waiting
    void stop() override;
};
