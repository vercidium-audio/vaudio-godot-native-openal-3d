// AI-created

#include "va_audio_backend.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "va_player.h"
#include "va_audio_backend_registry.h"
#include "va_device_name.h"

using namespace godot;

static VAAudioBackend *singleton = nullptr;

VAAudioBackend *VAAudioBackend::get_singleton()
{
    return singleton;
}

VAAudioBackend::VAAudioBackend()
{
    singleton = this;
}

VAAudioBackend::~VAAudioBackend()
{
    shutdown();

    if (singleton == this)
        singleton = nullptr;
}

void VAAudioBackend::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_backend_name"), &VAAudioBackend::get_backend_name);
    ClassDB::bind_method(D_METHOD("is_initialized"), &VAAudioBackend::is_initialized);
    ClassDB::bind_method(D_METHOD("get_error_count"), &VAAudioBackend::get_error_count);
    ClassDB::bind_method(D_METHOD("refresh_device_lists"), &VAAudioBackend::refresh_device_lists);
    ClassDB::bind_method(D_METHOD("get_reverb_send_level", "player"), &VAAudioBackend::get_reverb_send_level);

    ClassDB::bind_method(D_METHOD("get_available_devices"), &VAAudioBackend::get_available_devices);
    ClassDB::bind_method(D_METHOD("get_available_capture_devices"), &VAAudioBackend::get_available_capture_devices);
    ClassDB::bind_method(D_METHOD("set_output_device", "device_name"), &VAAudioBackend::set_output_device);

    ClassDB::bind_method(D_METHOD("get_available_hrtfs"), &VAAudioBackend::get_available_hrtfs);
    ClassDB::bind_method(D_METHOD("set_hrtf", "name"), &VAAudioBackend::set_hrtf);
    ClassDB::bind_method(D_METHOD("get_hrtf"), &VAAudioBackend::get_hrtf);
    ClassDB::bind_method(D_METHOD("get_active_hrtf"), &VAAudioBackend::get_active_hrtf);
    ClassDB::bind_method(D_METHOD("set_hrtf_enabled", "value"), &VAAudioBackend::set_hrtf_enabled);
    ClassDB::bind_method(D_METHOD("get_hrtf_enabled"), &VAAudioBackend::get_hrtf_enabled);

    ClassDB::bind_method(D_METHOD("set_master_volume", "value"), &VAAudioBackend::set_master_volume);
    ClassDB::bind_method(D_METHOD("get_master_volume"), &VAAudioBackend::get_master_volume);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "master_volume"), "set_master_volume", "get_master_volume");

    ClassDB::bind_method(D_METHOD("set_distance_model", "value"), &VAAudioBackend::set_distance_model);
    ClassDB::bind_method(D_METHOD("get_distance_model"), &VAAudioBackend::get_distance_model);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "distance_model", PROPERTY_HINT_ENUM, VA_DISTANCE_MODEL_HINT), "set_distance_model", "get_distance_model");

    ClassDB::bind_method(D_METHOD("set_meters_per_unit", "value"), &VAAudioBackend::set_meters_per_unit);
    ClassDB::bind_method(D_METHOD("get_meters_per_unit"), &VAAudioBackend::get_meters_per_unit);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "meters_per_unit"), "set_meters_per_unit", "get_meters_per_unit");

    ClassDB::bind_method(D_METHOD("set_speed_of_sound", "value"), &VAAudioBackend::set_speed_of_sound);
    ClassDB::bind_method(D_METHOD("get_speed_of_sound"), &VAAudioBackend::get_speed_of_sound);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_of_sound"), "set_speed_of_sound", "get_speed_of_sound");

    ClassDB::bind_method(D_METHOD("set_doppler_scale", "value"), &VAAudioBackend::set_doppler_scale);
    ClassDB::bind_method(D_METHOD("get_doppler_scale"), &VAAudioBackend::get_doppler_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "doppler_scale"), "set_doppler_scale", "get_doppler_scale");

    ClassDB::bind_method(D_METHOD("set_reverb_only", "value"), &VAAudioBackend::set_reverb_only);
    ClassDB::bind_method(D_METHOD("get_reverb_only"), &VAAudioBackend::get_reverb_only);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reverb_only"), "set_reverb_only", "get_reverb_only");
}

bool VAAudioBackend::initialize()
{
    if (!backend)
        backend = VAAudioBackendRegistry::create_selected();

    return backend && backend->initialise();
}

void VAAudioBackend::shutdown()
{
    if (backend)
        backend->shutdown();

    backend.reset();
}

void VAAudioBackend::update()
{
    if (is_initialized())
        backend->update();
}

String VAAudioBackend::get_backend_name() const
{
    return backend ? String(backend->get_name()) : String();
}

static void log_no_backend(const char *action)
{
    VA_BACKEND_ERROR("Can't ", action, " because no audio backend is available. Check the earlier vaudio errors in the Output panel");
}

std::unique_ptr<VASound> VAAudioBackend::create_sound(const int16_t *pcm16, int64_t sample_count, int channels, int sample_rate)
{
    if (!backend)
    {
        log_no_backend("load sound");
        return nullptr;
    }

    return backend->create_sound(pcm16, sample_count, channels, sample_rate);
}

std::unique_ptr<VAStream> VAAudioBackend::create_stream(VAPcmFormat format, int sample_rate)
{
    if (!backend)
    {
        log_no_backend("open a stream");
        return nullptr;
    }

    return backend->create_stream(format, sample_rate);
}

std::unique_ptr<VAReverbEffect> VAAudioBackend::create_reverb_effect()
{
    return backend ? backend->create_reverb_effect() : nullptr;
}

std::unique_ptr<VACaptureDevice> VAAudioBackend::open_capture_device(const String &device_name, VAPcmFormat format, int sample_rate, int buffer_frames, VACaptureCallback on_data)
{
    if (!backend)
    {
        log_no_backend("open a capture device");
        return nullptr;
    }

    return backend->open_capture_device(device_name, format, sample_rate, buffer_frames, std::move(on_data));
}

float VAAudioBackend::get_reverb_send_level(Node *player) const
{
    VAPlayer *source = Object::cast_to<VAPlayer>(player);
    VAVoice *voice = source ? source->get_last_voice() : nullptr;
    return voice ? voice->get_reverb_send_level() : -1.0f;
}

void VAAudioBackend::refresh_device_lists()
{
    PackedStringArray devices = get_available_devices();
    devices.insert(0, DEFAULT_DEVICE_LABEL);

    Dictionary output_device_info;
    output_device_info["name"] = "audio/vaudio/output_device";
    output_device_info["type"] = Variant::STRING;
    output_device_info["hint"] = PROPERTY_HINT_ENUM;
    output_device_info["hint_string"] = String(",").join(devices);
    ProjectSettings::get_singleton()->add_property_info(output_device_info);

    if (backend)
        backend->refresh_setting_hints();
}

PackedStringArray VAAudioBackend::get_available_devices()
{
    return backend ? backend->get_output_devices() : PackedStringArray();
}

PackedStringArray VAAudioBackend::get_available_capture_devices()
{
    return backend ? backend->get_capture_devices() : PackedStringArray();
}

const PackedStringArray &VAAudioBackend::get_cached_capture_devices()
{
    if (!capture_devices_cached)
    {
        cached_capture_devices = get_available_capture_devices();
        capture_devices_cached = true;
    }

    return cached_capture_devices;
}

bool VAAudioBackend::set_output_device(const String &device_name)
{
    return backend && backend->set_output_device(device_name);
}

PackedStringArray VAAudioBackend::get_available_hrtfs()
{
    return backend ? backend->get_available_hrtfs() : PackedStringArray();
}

bool VAAudioBackend::set_hrtf(const String &name)
{
    return backend && backend->set_hrtf(name);
}

String VAAudioBackend::get_hrtf() const
{
    return backend ? backend->get_hrtf() : String();
}

String VAAudioBackend::get_active_hrtf()
{
    return backend ? backend->get_active_hrtf() : String();
}

bool VAAudioBackend::set_hrtf_enabled(bool value)
{
    return backend && backend->set_hrtf_enabled(value);
}

bool VAAudioBackend::get_hrtf_enabled() const
{
    return backend && backend->get_hrtf_enabled();
}

void VAAudioBackend::set_master_volume(float value)
{
    if (backend)
        backend->set_master_volume(value);
}

float VAAudioBackend::get_master_volume() const
{
    return backend ? backend->get_master_volume() : 1.0f;
}

void VAAudioBackend::set_distance_model(int value)
{
    if (backend)
        backend->set_distance_model(static_cast<VADistanceModel>(value));
}

int VAAudioBackend::get_distance_model() const
{
    return static_cast<int>(backend ? backend->get_distance_model() : VADistanceModel::InverseClamped);
}

void VAAudioBackend::set_meters_per_unit(float value)
{
    if (backend)
        backend->set_meters_per_unit(value);
}

float VAAudioBackend::get_meters_per_unit() const
{
    return backend ? backend->get_meters_per_unit() : 1.0f;
}

void VAAudioBackend::set_speed_of_sound(float value)
{
    if (backend)
        backend->set_speed_of_sound(value);
}

float VAAudioBackend::get_speed_of_sound() const
{
    return backend ? backend->get_speed_of_sound() : 343.0f;
}

void VAAudioBackend::set_doppler_scale(float value)
{
    if (backend)
        backend->set_doppler_scale(value);
}

float VAAudioBackend::get_doppler_scale() const
{
    return backend ? backend->get_doppler_scale() : 1.0f;
}

void VAAudioBackend::set_listener_position(const Vector3 &position)
{
    if (backend)
        backend->set_listener_position(position);
}

void VAAudioBackend::set_listener_orientation(const Vector3 &forward, const Vector3 &up)
{
    if (backend)
        backend->set_listener_orientation(forward, up);
}
