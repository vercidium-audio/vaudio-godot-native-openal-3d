// AI-created

#pragma once

#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

#include "backend/va_audio_backend_base.h"
#include "fmod_api.h"

using namespace godot;

class FMODVoice;

// Audio backend powered by FMOD Core. Mirrors the C# plugin's FMODBackend
class FMODBackend : public VAAudioBackendBase
{
private:
    FMOD_SYSTEM *system = nullptr;
    FMOD_CHANNELGROUP *master_group = nullptr;

    // initialise() logs a failure once, rather than again for every VAWorld that enters the tree
    bool initialise_failed = false;
    bool library_loaded = false;

    // Read once, so a runtime set_output_device isn't overwritten. Empty device name means the system default
    bool settings_read = false;
    String output_device_name;
    String library_path;
    int sample_rate = 0;
    int max_channels = 512;
    int dsp_buffer_size = 0;

    // FMOD rejects every listener update while these aren't unit length and perpendicular, and apply_all_settings sets the position first
    FMOD_VECTOR listener_position = {0, 0, 0};
    FMOD_VECTOR listener_velocity = {0, 0, 0};
    FMOD_VECTOR listener_forward = {0, 0, -1};
    FMOD_VECTOR listener_up = {0, 1, 0};

    // Every live voice, so a distance model change reaches them all, and shutdown can detach them before the system is released
    std::unordered_set<FMODVoice *> voices;

    void ensure_settings_read();
    bool load_library();

    // Creates and initialises the FMOD system with the project settings. Logs and returns false on failure
    bool create_system();

    void select_driver(FMOD_SYSTEM *target);
    PackedStringArray get_driver_names(FMOD_SYSTEM *target);

    struct RecordDriver
    {
        int index;
        String name;
        bool connected;
        bool is_default;
    };

    std::vector<RecordDriver> get_record_drivers(FMOD_SYSTEM *target);

    // The live system, or (in the editor or after a failed start) a temporary one that lists the drivers. Record drivers are only enumerated once it's initialised (FMOD returns ERR_UNINITIALIZED before), which opens the output device briefly
    PackedStringArray with_system(const std::function<PackedStringArray(FMOD_SYSTEM *)> &list, bool initialise_temporary);

    void apply_3d_settings();
    void apply_listener();

protected:
    void apply_master_volume(float volume) override;
    void apply_distance_model(VADistanceModel model) override;
    void apply_meters_per_unit(float value) override;
    void apply_speed_of_sound(float value) override;
    void apply_doppler_scale(float scale) override;
    void apply_listener_position(const Vector3 &position) override;
    void apply_listener_velocity(const Vector3 &velocity) override;
    void apply_listener_orientation(const Vector3 &forward, const Vector3 &up) override;

public:
    static constexpr const char *BACKEND_NAME = "FMOD";

    // The initialised FMOD backend, or nullptr if FMOD isn't running
    static FMODBackend *get_singleton();

    // Logs a failed FMOD call and counts it as a backend error. Returns whether it succeeded
    static bool check(FMOD_RESULT result, const char *call);

    static FMOD_VECTOR to_fmod(const Vector3 &v)
    {
        return {v.x, v.y, v.z};
    }

    static FMOD_SOUND_FORMAT to_fmod(VAPcmFormat format);

    // FMOD's PCM8 is signed, VAPcmFormat's 8-bit formats are unsigned like OpenAL's, so this converts either way
    static void toggle_sign8(uint8_t *data, unsigned int length);

    FMODBackend() = default;
    ~FMODBackend() override;

    const char *get_name() const override
    {
        return BACKEND_NAME;
    }

    bool is_initialised() const override
    {
        return system != nullptr;
    }

    bool initialise() override;
    void update() override;
    void shutdown() override;

    PackedStringArray get_output_devices() override;
    bool set_output_device(const String &name) override;
    PackedStringArray get_capture_devices() override;

    // FMOD has no backend-specific dropdowns
    void refresh_setting_hints() override {}

    std::unique_ptr<VASound> create_sound(const int16_t *pcm16, int64_t sample_count, int channels, int sample_rate) override;
    std::unique_ptr<VAStream> create_stream(VAPcmFormat format, int sample_rate) override;
    std::unique_ptr<VAReverbEffect> create_reverb_effect() override;
    std::unique_ptr<VACaptureDevice> open_capture_device(const String &device_name, VAPcmFormat format, int sample_rate, int buffer_frames, VACaptureCallback on_data) override;

    FMOD_SYSTEM *get_system() const
    {
        return system;
    }

    // Plays the sound paused on a new channel. stream_sound is a stream's sound, which the voice then owns. Returns nullptr if FMOD failed, which it logs
    std::unique_ptr<FMODVoice> play_sound(FMOD_SOUND *sound, int sound_sample_rate, float length, bool spatialised, FMOD_SOUND *stream_sound = nullptr);

    void add_voice(FMODVoice *voice)
    {
        voices.insert(voice);
    }

    void remove_voice(FMODVoice *voice)
    {
        voices.erase(voice);
    }
};
