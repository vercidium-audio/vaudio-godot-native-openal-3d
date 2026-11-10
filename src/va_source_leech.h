#pragma once

#include "al_source3d.h"
#include "va_bounds_playback.h"

extern "C"
{
#include <vaudio.h>
}

using namespace godot;

namespace va_godot
{
class VAWorld;
class VAEmitter;
}

class VASourceLeech : public ALSource3D
{
    GDCLASS(VASourceLeech, ALSource3D);

private:
    va_godot::VAWorld *va_world = nullptr;

    // Not owned - the parent VAEmitter this node leeches raytracing results off, resolved once in _enter_tree via get_parent().
    va_godot::VAEmitter *emitter = nullptr;

    VABoundsPlayback playback;

    void apply_raytracing_results(va_godot::VAEmitter *other);

protected:
    static void _bind_methods();

public:
    VASourceLeech();
    ~VASourceLeech();

    void _enter_tree() override;
    void _exit_tree() override;

    // Autoplay waits for the parent emitter to be ready to play and inside the bounds
    void _ready() override;

    void _process(double delta) override;

    bool is_raytraced() const;

    // See VAEmitter::is_ready_to_play
    bool is_ready_to_play() const;

    // The parent emitter's VAEmitter::is_silenced_by_bounds, using its clamp_position
    bool is_silenced_by_bounds() const;

    // Matches VASource's Play() override: returns false without playing until the parent emitter is ready to play, and follows the same bounds rules.
    bool play() override;

    // Also cancels a play that's waiting, e.g. a loop waiting outside the bounds
    void stop() override;

    // Current muffling filter state (direct/dry path only); 1.0/1.0 until the listener has raytraced the parent emitter at least once.
    float get_muffling_gain_lf() const;
    float get_muffling_gain_hf() const;
};
