// AI-created

#pragma once

#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>

#include "va_audio_objects.h"
#include "va_audio_types.h"
#include "va_engine_util.h"

// Logs a failed backend call and counts it in VAAudioBackendBase::get_error_count(), so tests can check a run hit no backend errors
#define VA_BACKEND_ERROR(...) (VAAudioBackendBase::count_error(), VA_ERROR(__VA_ARGS__))

// Receives captured audio: a pointer to the frames and the frame count. The pointer is only valid during the call
using VACaptureCallback = std::function<void(const uint8_t *, int)>;

// An audio library that plays vaudio's sounds (OpenAL, FMOD, ...). Exactly one is used per game, chosen by the audio/vaudio/backend project setting. Implementations register themselves with VAAudioBackendRegistry. The global mix settings are cached here: setting one stores it, and applies it straight away when initialised
class VAAudioBackendBase
{
public:
    virtual ~VAAudioBackendBase() = default;

    // The audio/vaudio/backend value that selects this backend
    virtual const char *get_name() const = 0;

    virtual bool is_initialised() const = 0;

    // Opens the output device. Returns false (with the reason logged) if it failed
    virtual bool initialise() = 0;

    // Called once per frame while initialised
    virtual void update() = 0;

    // Calls on_device_destroying, then closes the output device
    virtual void shutdown() = 0;

    // Static, so errors logged before the backend exists or from worker threads still count
    static void count_error()
    {
        error_count.fetch_add(1, std::memory_order_relaxed);
    }

    // How many backend errors have been logged since the game started
    static int64_t get_error_count()
    {
        return error_count.load(std::memory_order_relaxed);
    }

    // Called before the device is destroyed (e.g. when a device switch has to fully recreate it), after which every voice, sound and reverb effect is invalid
    std::function<void()> on_device_destroying;

    // Called once the device has been recreated
    std::function<void()> on_device_recreated;

    float get_master_volume() const
    {
        return master_volume;
    }

    void set_master_volume(float value);

    VADistanceModel get_distance_model() const
    {
        return distance_model;
    }

    void set_distance_model(VADistanceModel value);

    float get_meters_per_unit() const
    {
        return meters_per_unit;
    }

    void set_meters_per_unit(float value);

    float get_speed_of_sound() const
    {
        return speed_of_sound;
    }

    void set_speed_of_sound(float value);

    float get_doppler_scale() const
    {
        return doppler_scale;
    }

    void set_doppler_scale(float value);

    const godot::Vector3 &get_listener_position() const
    {
        return listener_position;
    }

    void set_listener_position(const godot::Vector3 &position);
    void set_listener_velocity(const godot::Vector3 &velocity);
    void set_listener_orientation(const godot::Vector3 &forward, const godot::Vector3 &up);

    virtual godot::PackedStringArray get_output_devices() = 0;

    // Switches to the named output device, or the system default when empty
    virtual bool set_output_device(const godot::String &name) = 0;

    virtual godot::PackedStringArray get_capture_devices() = 0;

    // Refreshes the dropdowns of this backend's own project settings. Called in the editor
    virtual void refresh_setting_hints() = 0;

    // Uploads decoded 16-bit PCM (sample_count samples, interleaved). Can be called from a worker thread. Returns nullptr if the device isn't available
    virtual std::unique_ptr<VASound> create_sound(const int16_t *pcm16, int64_t sample_count, int channels, int sample_rate) = 0;

    // Creates a stream that plays audio pushed into it. Returns nullptr (with the reason logged) when out of voices
    virtual std::unique_ptr<VAStream> create_stream(VAPcmFormat format, int sample_rate) = 0;

    // Creates a reverb effect that's silent until its first VAReverbEffect::set_params
    virtual std::unique_ptr<VAReverbEffect> create_reverb_effect() = 0;

    // Opens a capture device (empty name = system default). Returns nullptr (with the reason logged) if it failed to open
    virtual std::unique_ptr<VACaptureDevice> open_capture_device(const godot::String &device_name, VAPcmFormat format, int sample_rate, int buffer_frames, VACaptureCallback on_data) = 0;

    // HRTF, which only OpenAL Soft supports. Backends without it keep these defaults
    virtual godot::PackedStringArray get_available_hrtfs()
    {
        return godot::PackedStringArray();
    }

    virtual bool set_hrtf(const godot::String &name)
    {
        return false;
    }

    virtual godot::String get_hrtf() const
    {
        return godot::String();
    }

    virtual godot::String get_active_hrtf()
    {
        return godot::String();
    }

    virtual bool set_hrtf_enabled(bool value)
    {
        return false;
    }

    virtual bool get_hrtf_enabled() const
    {
        return false;
    }

protected:
    void raise_device_destroying()
    {
        if (on_device_destroying)
            on_device_destroying();
    }

    void raise_device_recreated()
    {
        if (on_device_recreated)
            on_device_recreated();
    }

    // Pushes every cached setting to a newly created device
    void apply_all_settings();

    virtual void apply_master_volume(float volume) = 0;
    virtual void apply_distance_model(VADistanceModel model) = 0;
    virtual void apply_meters_per_unit(float value) = 0;
    virtual void apply_speed_of_sound(float value) = 0;
    virtual void apply_doppler_scale(float scale) = 0;
    virtual void apply_listener_position(const godot::Vector3 &position) = 0;
    virtual void apply_listener_velocity(const godot::Vector3 &velocity) = 0;
    virtual void apply_listener_orientation(const godot::Vector3 &forward, const godot::Vector3 &up) = 0;

private:
    inline static std::atomic<int64_t> error_count{0};

    float master_volume = 1.0f;
    VADistanceModel distance_model = VADistanceModel::InverseClamped;
    float meters_per_unit = 1.0f;
    float speed_of_sound = 343.0f;
    float doppler_scale = 1.0f;

    godot::Vector3 listener_position;
    godot::Vector3 listener_velocity;
    godot::Vector3 listener_forward = godot::Vector3(0, 0, -1);
    godot::Vector3 listener_up = godot::Vector3(0, 1, 0);
};
