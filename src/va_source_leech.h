// Human-reviewed

#pragma once

#include "al_source3d.h"
#include "va_bounds_playback.h"

extern "C"
{
#include <vaudio.h>
}

using namespace godot;

namespace va_godot { class VAEmitter; }

class VASourceLeech : public ALSource3D
{
    GDCLASS(VASourceLeech, ALSource3D);

private:
    // Not owned - the parent VAEmitter this node leeches raytracing results off, resolved once in _enter_tree via get_parent().
    va_godot::VAEmitter *emitter = nullptr;

    VABoundsPlayback playback;

protected:
    static void _bind_methods();

public:
    VASourceLeech();
    ~VASourceLeech();

    void _enter_tree() override;
    void _exit_tree() override;

    // Autoplay waits for the parent emitter to be ready to play and within world bounds
    void _ready() override;

    void _process(double delta) override;

    // True if the parent VAEmitter has cast its own reverb and ambience rays
    bool is_raytraced() const;

    // See VAEmitter::is_ready_to_play
    bool is_ready_to_play() const;

    // True if the parent VAEmitter is outside the world bounds, and VAWorld.EmittersOutsideTheWorldAreMuffled=true
    bool is_silenced_by_bounds() const;

    // Matches VASource's Play() override: waits until the parent emitter is ready to play, and follows the same bounds rules
    bool play(float start_time = 0.0f) override;

    // Stop playback and cancel pending playback
    void stop() override;

    // Current muffling filter state (direct/dry path only); 1.0/1.0 until the listener has raytraced the parent emitter at least once.
    float get_muffling_gain_lf() const;
    float get_muffling_gain_hf() const;
};
