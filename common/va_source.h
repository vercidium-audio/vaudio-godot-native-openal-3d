#pragma once

#include "va_bounds_playback.h"
#include "va_raytraced_source.h"

using namespace godot;

class VASource : public VARaytracedSource
{
    GDCLASS(VASource, VARaytracedSource);

private:
    VABoundsPlayback playback;

protected:
    static void _bind_methods();

public:
    VASource();
    ~VASource();

    // Autoplay waits for _process, once the source is ready to play and inside the bounds
    void _ready() override;

    void _process(double delta) override;

    bool play() override;

    // Also cancels a play that's waiting, e.g. a loop waiting outside the bounds
    void stop() override;
};
