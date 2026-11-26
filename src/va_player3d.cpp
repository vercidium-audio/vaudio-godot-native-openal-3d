// AI-created

#include "va_player3d.h"

#include <godot_cpp/core/class_db.hpp>

void VAPlayer3D::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_max_distance"), &VAPlayer3D::get_max_distance);
    ClassDB::bind_method(D_METHOD("set_max_distance", "value"), &VAPlayer3D::set_max_distance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "0.0,1000.0,0.1,or_greater"), "set_max_distance", "get_max_distance");

    ClassDB::bind_method(D_METHOD("get_reference_distance"), &VAPlayer3D::get_reference_distance);
    ClassDB::bind_method(D_METHOD("set_reference_distance", "value"), &VAPlayer3D::set_reference_distance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reference_distance", PROPERTY_HINT_RANGE, "0.0,1000.0,0.1,or_greater"), "set_reference_distance", "get_reference_distance");

    ClassDB::bind_method(D_METHOD("get_rolloff_factor"), &VAPlayer3D::get_rolloff_factor);
    ClassDB::bind_method(D_METHOD("set_rolloff_factor", "value"), &VAPlayer3D::set_rolloff_factor);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rolloff_factor", PROPERTY_HINT_RANGE, "0.0,10.0,0.1,or_greater"), "set_rolloff_factor", "get_rolloff_factor");

    // Script-only alias for `reference_distance` - not exposed in the inspector, see va_player3d.h's get_unit_size().
    ClassDB::bind_method(D_METHOD("get_unit_size"), &VAPlayer3D::get_unit_size);
    ClassDB::bind_method(D_METHOD("set_unit_size", "value"), &VAPlayer3D::set_unit_size);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "unit_size", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_unit_size", "get_unit_size");
}

VAPlayer3D::VAPlayer3D()
{
}

VAPlayer3D::~VAPlayer3D()
{
}

void VAPlayer3D::configure_voice(VAVoice &voice)
{
    voice.set_max_distance(max_distance);
    voice.set_reference_distance(reference_distance);
    voice.set_rolloff(rolloff_factor);
    voice.set_position(get_global_position());
}

void VAPlayer3D::_process(double delta)
{
    VAPlayer::_process(delta);

    // Keeps every live voice's position in sync each frame; play() only sets it once, so without this a voice's audible position freezes.
    Vector3 position = get_global_position();

    for (auto &voice : voices)
        voice.get()->set_position(position);
}

void VAPlayer3D::set_max_distance(float value)
{
    max_distance = value;

    for (auto &voice : voices)
        voice.get()->set_max_distance(value);
}

void VAPlayer3D::set_reference_distance(float value)
{
    reference_distance = value;

    for (auto &voice : voices)
        voice.get()->set_reference_distance(value);
}

void VAPlayer3D::set_rolloff_factor(float value)
{
    rolloff_factor = value;

    for (auto &voice : voices)
        voice.get()->set_rolloff(value);
}
