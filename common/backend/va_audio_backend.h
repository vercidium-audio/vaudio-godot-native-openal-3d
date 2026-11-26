// AI-created

#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <memory>

#include "va_audio_backend_base.h"

using namespace godot;

// Engine singleton over the audio backend selected by audio/vaudio/backend, reachable from GDScript as VAAudioBackend. Nodes go through this rather than a backend type. Same name as the C# plugin's VAAudioBackend
class VAAudioBackend : public Object
{
    GDCLASS(VAAudioBackend, Object);

private:
    std::unique_ptr<VAAudioBackendBase> backend;

    bool reverb_only = false;

    // See get_cached_capture_devices()
    PackedStringArray cached_capture_devices;
    bool capture_devices_cached = false;

protected:
    static void _bind_methods();

public:
    // Created at module init, so it exists whenever the plugin is loaded, even if the backend failed to start
    static VAAudioBackend *get_singleton();

    VAAudioBackend();
    ~VAAudioBackend();

    // Creates the selected backend and opens its device (in the editor too, for the device and HRTF dropdowns). Returns false if it failed to start
    bool initialize();

    void shutdown();

    // Called once per frame by every VAWorld
    void update();

    // nullptr if no backend is compiled in
    VAAudioBackendBase *get_backend() const
    {
        return backend.get();
    }

    bool is_initialized() const
    {
        return backend && backend->is_initialised();
    }

    // The selected backend's name, e.g. "OpenAL", or empty if none is available
    String get_backend_name() const;

    // The backend's factories, which log a counted error and return nullptr when there's no backend. See VAAudioBackendBase
    std::unique_ptr<VASound> create_sound(const int16_t *pcm16, int64_t sample_count, int channels, int sample_rate);
    std::unique_ptr<VAStream> create_stream(VAPcmFormat format, int sample_rate);
    std::unique_ptr<VAReverbEffect> create_reverb_effect();
    std::unique_ptr<VACaptureDevice> open_capture_device(const String &device_name, VAPcmFormat format, int sample_rate, int buffer_frames, VACaptureCallback on_data);

    // The reverb send level (0-1) of a player node's most recent voice, or -1 when it has none or the backend doesn't report it. For tests
    float get_reverb_send_level(Node *player) const;

    int64_t get_error_count() const
    {
        return VAAudioBackendBase::get_error_count();
    }

    // Fills the audio/vaudio/output_device dropdown and the backend's own setting dropdowns
    void refresh_device_lists();

    PackedStringArray get_available_devices();
    PackedStringArray get_available_capture_devices();

    // Enumerating capture devices can be slow, and Godot calls _validate_property every time it fetches the property list, so VAInputStreamSource's dropdown reuses this until refresh_capture_devices()
    const PackedStringArray &get_cached_capture_devices();

    void refresh_capture_devices()
    {
        capture_devices_cached = false;
    }

    bool set_output_device(const String &device_name);

    PackedStringArray get_available_hrtfs();
    bool set_hrtf(const String &name);
    String get_hrtf() const;
    String get_active_hrtf();
    bool set_hrtf_enabled(bool value);
    bool get_hrtf_enabled() const;

    void set_master_volume(float value);
    float get_master_volume() const;

    void set_distance_model(int value);
    int get_distance_model() const;

    void set_meters_per_unit(float value);
    float get_meters_per_unit() const;

    void set_speed_of_sound(float value);
    float get_speed_of_sound() const;

    void set_doppler_scale(float value);
    float get_doppler_scale() const;

    // Node logic rather than a backend setting: sources silence their own direct path
    void set_reverb_only(bool value)
    {
        reverb_only = value;
    }

    bool get_reverb_only() const
    {
        return reverb_only;
    }

    void set_listener_position(const Vector3 &position);
    void set_listener_orientation(const Vector3 &forward, const Vector3 &up);
};
