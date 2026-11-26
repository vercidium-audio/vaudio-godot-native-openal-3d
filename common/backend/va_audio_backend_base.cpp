// AI-created

#include "va_audio_backend_base.h"

#include <algorithm>

using namespace godot;

void VAAudioBackendBase::set_master_volume(float value)
{
    value = std::max(0.0f, value);

    if (value == master_volume)
        return;

    master_volume = value;

    if (is_initialised())
        apply_master_volume(value);
}

void VAAudioBackendBase::set_distance_model(VADistanceModel value)
{
    if (value == distance_model)
        return;

    distance_model = value;

    if (is_initialised())
        apply_distance_model(value);
}

void VAAudioBackendBase::set_meters_per_unit(float value)
{
    value = std::max(0.0f, value);

    if (value == meters_per_unit)
        return;

    meters_per_unit = value;

    if (is_initialised())
        apply_meters_per_unit(value);
}

void VAAudioBackendBase::set_speed_of_sound(float value)
{
    value = std::max(0.0f, value);

    if (value == speed_of_sound)
        return;

    speed_of_sound = value;

    if (is_initialised())
        apply_speed_of_sound(value);
}

void VAAudioBackendBase::set_doppler_scale(float value)
{
    value = std::max(0.0f, value);

    if (value == doppler_scale)
        return;

    doppler_scale = value;

    if (is_initialised())
        apply_doppler_scale(value);
}

void VAAudioBackendBase::set_listener_position(const Vector3 &position)
{
    if (position == listener_position)
        return;

    listener_position = position;

    if (is_initialised())
        apply_listener_position(position);
}

void VAAudioBackendBase::set_listener_velocity(const Vector3 &velocity)
{
    if (velocity == listener_velocity)
        return;

    listener_velocity = velocity;

    if (is_initialised())
        apply_listener_velocity(velocity);
}

void VAAudioBackendBase::set_listener_orientation(const Vector3 &forward, const Vector3 &up)
{
    if (forward == listener_forward && up == listener_up)
        return;

    listener_forward = forward;
    listener_up = up;

    if (is_initialised())
        apply_listener_orientation(forward, up);
}

void VAAudioBackendBase::apply_all_settings()
{
    apply_master_volume(master_volume);
    apply_distance_model(distance_model);
    apply_meters_per_unit(meters_per_unit);
    apply_speed_of_sound(speed_of_sound);
    apply_doppler_scale(doppler_scale);
    apply_listener_position(listener_position);
    apply_listener_velocity(listener_velocity);
    apply_listener_orientation(listener_forward, listener_up);
}
