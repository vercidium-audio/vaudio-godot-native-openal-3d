// AI-created

#pragma once

#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <string>

#include "backend/va_audio_backend_base.h"
#include "backend/va_dynamic_library.h"
#include "al_filter.h"
#include "al_functions.h"

using namespace godot;

// Audio backend powered by OpenAL Soft. The AL helper classes in this folder (ALSourceHandle, ALFilter, ...) reach the loaded functions through get_singleton()
class OpenALBackend : public VAAudioBackendBase
{
private:
    va_dynamic_library library = nullptr;

    ALCdevice *device = nullptr;
    ALCcontext *context = nullptr;

    alcOpenDeviceFn alcOpenDevice_ = nullptr;
    alcCloseDeviceFn alcCloseDevice_ = nullptr;
    alcCreateContextFn alcCreateContext_ = nullptr;
    alcMakeContextCurrentFn alcMakeContextCurrent_ = nullptr;
    alcDestroyContextFn alcDestroyContext_ = nullptr;
    alcGetErrorFn alcGetError_ = nullptr;
    alcGetStringFn alcGetString_ = nullptr;
    alcGetIntegervFn alcGetIntegerv_ = nullptr;
    alcIsExtensionPresentFn alcIsExtensionPresent_ = nullptr;
    alcGetProcAddressFn alcGetProcAddress_ = nullptr;

    alcReopenDeviceSOFTFn alcReopenDeviceSOFT_ = nullptr;
    alcGetStringiSOFTFn alcGetStringiSOFT_ = nullptr;
    alcResetDeviceSOFTFn alcResetDeviceSOFT_ = nullptr;

    alcCaptureOpenDeviceFn alcCaptureOpenDevice_ = nullptr;
    alcCaptureCloseDeviceFn alcCaptureCloseDevice_ = nullptr;
    alcCaptureStartFn alcCaptureStart_ = nullptr;
    alcCaptureStopFn alcCaptureStop_ = nullptr;
    alcCaptureSamplesFn alcCaptureSamples_ = nullptr;

    alGetErrorFn alGetError_ = nullptr;
    alGetStringFn alGetString_ = nullptr;
    alDistanceModelFn alDistanceModel_ = nullptr;
    alSpeedOfSoundFn alSpeedOfSound_ = nullptr;
    alDopplerFactorFn alDopplerFactor_ = nullptr;
    alListenerfFn alListenerf_ = nullptr;
    alListenerfvFn alListenerfv_ = nullptr;

    alGenBuffersFn alGenBuffers_ = nullptr;
    alDeleteBuffersFn alDeleteBuffers_ = nullptr;
    alIsBufferFn alIsBuffer_ = nullptr;
    alGetBufferiFn alGetBufferi_ = nullptr;
    alBufferDataFn alBufferData_ = nullptr;

    alGenSourcesFn alGenSources_ = nullptr;
    alDeleteSourcesFn alDeleteSources_ = nullptr;
    alIsSourceFn alIsSource_ = nullptr;
    alSourceiFn alSourcei_ = nullptr;
    alSourcefFn alSourcef_ = nullptr;
    alSourcefvFn alSourcefv_ = nullptr;
    alGetSourceiFn alGetSourcei_ = nullptr;
    alGetSourcefFn alGetSourcef_ = nullptr;
    alSourcePlayFn alSourcePlay_ = nullptr;
    alSourceStopFn alSourceStop_ = nullptr;
    alSourcePauseFn alSourcePause_ = nullptr;
    alSource3iFn alSource3i_ = nullptr;

    alGetProcAddressFn alGetProcAddress_ = nullptr;

    alGenFiltersFn alGenFilters_ = nullptr;
    alDeleteFiltersFn alDeleteFilters_ = nullptr;
    alFilteriFn alFilteri_ = nullptr;
    alFilterfFn alFilterf_ = nullptr;

    alGenEffectsFn alGenEffects_ = nullptr;
    alDeleteEffectsFn alDeleteEffects_ = nullptr;
    alEffectiFn alEffecti_ = nullptr;
    alEffectfFn alEffectf_ = nullptr;
    alEffectfvFn alEffectfv_ = nullptr;

    alGenAuxiliaryEffectSlotsFn alGenAuxiliaryEffectSlots_ = nullptr;
    alDeleteAuxiliaryEffectSlotsFn alDeleteAuxiliaryEffectSlots_ = nullptr;
    alAuxiliaryEffectSlotiFn alAuxiliaryEffectSloti_ = nullptr;
    alAuxiliaryEffectSlotfFn alAuxiliaryEffectSlotf_ = nullptr;

    alBufferCallbackSOFTFn alBufferCallbackSOFT_ = nullptr;

    bool efx_present = false;

    std::string device_name;
    int max_auxiliary_sends = 0;

    int max_mono_sources = 0;
    int max_stereo_sources = 0;

    int sample_rate = 0;
    bool hrtf_enabled = false;

    // Empty means OpenAL Soft's default HRTF
    std::string hrtf_name;

    // check_device_connected() state, so a lost device is only reported once while it keeps retrying
    uint64_t next_connection_check_ms = 0;
    bool device_lost = false;

    bool load_library();
    bool resolve_functions();
    bool resolve_efx_functions();
    bool open_device_and_context();
    void unload_library();
    void close_device();

    // close_device() + open_device_and_context(), which invalidates every AL object, so it's surrounded by the device destroyed/recreated callbacks
    bool recreate_device();

    // Writes the zero-terminated ALC attribute list shared by context creation, device reopen and device reset. Needs at least 13 entries.
    void build_attributes(ALCint *attributes);

    // Index into the device's current HRTF list for ALC_HRTF_ID_SOFT, or -1 for OpenAL Soft's default
    int find_hrtf_index();

    // Points OpenAL Soft at the audio/vaudio/openal_hrtf_folder .mhr files. Must run before the library is loaded.
    void prepare_hrtf_folder();

    bool reset_device();

    bool reinitialize(const String &new_device_name);

    // Reads device_name/max_auxiliary_sends/sample_rate/hrtf_enabled/hrtf from Project Settings.
    void read_settings_from_project_settings();

    // Once the output device is lost (e.g. headphones unplugged), reopens it in place. Throttled to once a second
    void check_device_connected();

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
    static constexpr const char *BACKEND_NAME = "OpenAL";

    // The initialised OpenAL backend, or nullptr if OpenAL isn't running (not selected, failed to start, or shut down)
    static OpenALBackend *get_singleton();

    // The singleton, or nullptr after logging "Can't <action> because OpenAL failed to start" if OpenAL isn't running
    static OpenALBackend *get_initialized_singleton(const char *action);

    static ALenum to_al(VADistanceModel model);
    static ALenum to_al(VAPcmFormat format);

    OpenALBackend() = default;
    ~OpenALBackend() override;

    const char *get_name() const override
    {
        return BACKEND_NAME;
    }

    bool is_initialised() const override
    {
        return device && context;
    }

    bool initialise() override;
    void update() override;
    void shutdown() override;

    PackedStringArray get_output_devices() override;
    bool set_output_device(const String &name) override;
    PackedStringArray get_capture_devices() override;
    void refresh_setting_hints() override;

    std::unique_ptr<VASound> create_sound(const int16_t *pcm16, int64_t sample_count, int channels, int sample_rate) override;
    std::unique_ptr<VAStream> create_stream(VAPcmFormat format, int sample_rate) override;
    std::unique_ptr<VAReverbEffect> create_reverb_effect() override;
    std::unique_ptr<VACaptureDevice> open_capture_device(const String &device_name, VAPcmFormat format, int sample_rate, int buffer_frames, VACaptureCallback on_data) override;

    PackedStringArray get_available_hrtfs() override;
    bool set_hrtf(const String &name) override;

    String get_hrtf() const override
    {
        return String::utf8(hrtf_name.c_str());
    }

    String get_active_hrtf() override;
    bool set_hrtf_enabled(bool value) override;

    bool get_hrtf_enabled() const override
    {
        return hrtf_enabled;
    }

    // Generates one buffer, or logs a readable error and returns 0
    ALuint gen_buffer();

    // Generates a buffer and uploads 16-bit PCM into it, or logs a readable error and returns 0
    ALuint upload_pcm16(const int16_t *pcm16, int64_t sample_count, int channels, int frequency);

    void delete_buffer(ALuint handle)
    {
        alDeleteBuffers_(1, &handle);
    }

    // Deletes a sound's buffer, logging a counted error if a source still uses it. Does nothing once OpenAL has shut down, which deleted every buffer along with its device
    static void delete_sound_buffer(ALuint handle);

    void delete_source(ALuint handle)
    {
        alDeleteSources_(1, &handle);
    }

    void delete_filter(ALuint handle)
    {
        alDeleteFilters_(1, &handle);
    }

    void delete_effect(ALuint handle)
    {
        alDeleteEffects_(1, &handle);
    }

    void delete_auxiliary_effect_slot(ALuint handle)
    {
        alDeleteAuxiliaryEffectSlots_(1, &handle);
    }

    ALCdevice *get_device() const
    {
        return device;
    }

    ALCcontext *get_context() const
    {
        return context;
    }

    bool has_efx() const
    {
        return efx_present;
    }

    // OpenAL is loaded at runtime (see load_library), so these return the resolved function pointer and call sites read backend->al_sourcei()(args)
    alGenBuffersFn al_gen_buffers() const
    {
        return alGenBuffers_;
    }

    alDeleteBuffersFn al_delete_buffers() const
    {
        return alDeleteBuffers_;
    }

    alBufferDataFn al_buffer_data() const
    {
        return alBufferData_;
    }

    alGetErrorFn al_get_error() const
    {
        return alGetError_;
    }

    alcCaptureOpenDeviceFn alc_capture_open_device() const
    {
        return alcCaptureOpenDevice_;
    }

    alcCaptureCloseDeviceFn alc_capture_close_device() const
    {
        return alcCaptureCloseDevice_;
    }

    alcCaptureStartFn alc_capture_start() const
    {
        return alcCaptureStart_;
    }

    alcCaptureStopFn alc_capture_stop() const
    {
        return alcCaptureStop_;
    }

    alcCaptureSamplesFn alc_capture_samples() const
    {
        return alcCaptureSamples_;
    }

    alcGetIntegervFn alc_get_integerv() const
    {
        return alcGetIntegerv_;
    }

    alGenSourcesFn al_gen_sources() const
    {
        return alGenSources_;
    }

    alDeleteSourcesFn al_delete_sources() const
    {
        return alDeleteSources_;
    }

    alSourceiFn al_sourcei() const
    {
        return alSourcei_;
    }

    alSourcefFn al_sourcef() const
    {
        return alSourcef_;
    }

    alSourcefvFn al_sourcefv() const
    {
        return alSourcefv_;
    }

    alGetSourceiFn al_get_sourcei() const
    {
        return alGetSourcei_;
    }

    alSourcePlayFn al_source_play() const
    {
        return alSourcePlay_;
    }

    alSourceStopFn al_source_stop() const
    {
        return alSourceStop_;
    }

    alSourcePauseFn al_source_pause() const
    {
        return alSourcePause_;
    }

    alGetSourcefFn al_get_sourcef() const
    {
        return alGetSourcef_;
    }

    alGetBufferiFn al_get_bufferi() const
    {
        return alGetBufferi_;
    }

    alSource3iFn al_source3i() const
    {
        return alSource3i_;
    }

    alGenFiltersFn al_gen_filters() const
    {
        return alGenFilters_;
    }

    alDeleteFiltersFn al_delete_filters() const
    {
        return alDeleteFilters_;
    }

    alFilteriFn al_filteri() const
    {
        return alFilteri_;
    }

    alFilterfFn al_filterf() const
    {
        return alFilterf_;
    }

    alGenEffectsFn al_gen_effects() const
    {
        return alGenEffects_;
    }

    alDeleteEffectsFn al_delete_effects() const
    {
        return alDeleteEffects_;
    }

    alEffectiFn al_effecti() const
    {
        return alEffecti_;
    }

    alEffectfFn al_effectf() const
    {
        return alEffectf_;
    }

    alEffectfvFn al_effectfv() const
    {
        return alEffectfv_;
    }

    alGenAuxiliaryEffectSlotsFn al_gen_auxiliary_effect_slots() const
    {
        return alGenAuxiliaryEffectSlots_;
    }

    alDeleteAuxiliaryEffectSlotsFn al_delete_auxiliary_effect_slots() const
    {
        return alDeleteAuxiliaryEffectSlots_;
    }

    alAuxiliaryEffectSlotiFn al_auxiliary_effect_sloti() const
    {
        return alAuxiliaryEffectSloti_;
    }

    alAuxiliaryEffectSlotfFn al_auxiliary_effect_slotf() const
    {
        return alAuxiliaryEffectSlotf_;
    }

    alBufferCallbackSOFTFn al_buffer_callback_soft() const
    {
        return alBufferCallbackSOFT_;
    }
};
