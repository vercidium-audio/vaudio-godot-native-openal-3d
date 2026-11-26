// AI-created

#pragma once

#include "va_player.h"

using namespace godot;

// Spatialised player
class VAPlayer3D : public VAPlayer
{
    GDCLASS(VAPlayer3D, VAPlayer);

private:
    float max_distance = 100.0f;
    float reference_distance = 8.0f;
    float rolloff_factor = 1.0f;

protected:
    static void _bind_methods();

    void configure_voice(VAVoice &voice) override;

public:
    VAPlayer3D();
    ~VAPlayer3D();

    void _process(double delta) override;

    void set_max_distance(float value);
    void set_reference_distance(float value);
    void set_rolloff_factor(float value);

    float get_max_distance() const
    {
        return max_distance;
    }

    float get_reference_distance() const
    {
        return reference_distance;
    }

    float get_rolloff_factor() const
    {
        return rolloff_factor;
    }

    // Script-only alias for `reference_distance` matching AudioStreamPlayer3D's `unit_size` (see va_conversion_plugin.cpp's matching remap).
    float get_unit_size() const
    {
        return get_reference_distance();
    }

    void set_unit_size(float value)
    {
        set_reference_distance(value);
    }
};
