// AI-created

#include "openal_backend.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "backend/va_audio_backend_registry.h"
#include "openal_objects.h"
#include "va_device_name.h"
#include "va_engine_util.h"

#include <cstring>
#include <string>

using namespace godot;

// Registers with the backend registry when the plugin library loads, so core code never names OpenALBackend
static const bool registered = VAAudioBackendRegistry::register_backend(OpenALBackend::BACKEND_NAME, []() -> VAAudioBackendBase * { return new OpenALBackend(); });

static OpenALBackend *singleton = nullptr;

OpenALBackend *OpenALBackend::get_singleton()
{
    return singleton;
}

OpenALBackend *OpenALBackend::get_initialized_singleton(const char *action)
{
    if (singleton && singleton->is_initialised())
        return singleton;

    // The reason OpenAL didn't start was already logged at startup (or by the failed device switch)
    VA_BACKEND_ERROR("Can't ", action, " because OpenAL failed to start. Check the earlier vaudio errors in the Output panel, e.g. the OpenAL library is missing from the plugin's bin folder, or no audio output device was found");
    return nullptr;
}

ALuint OpenALBackend::gen_buffer()
{
    // alGetError is a single sticky flag, so clear any stale error before reading the result of alGenBuffers below
    alGetError_();

    ALuint new_handle = 0;
    alGenBuffers_(1, &new_handle);

    ALenum error = alGetError_();

    if (new_handle != 0 && error == AL_NO_ERROR)
        return new_handle;

    if (error == AL_OUT_OF_MEMORY)
        VA_BACKEND_ERROR("Failed to create an OpenAL buffer because the system is out of memory");
    else
        VA_BACKEND_ERROR("Failed to create an OpenAL buffer. Error code: ", ALErrorToString(error));

    return 0;
}

ALuint OpenALBackend::upload_pcm16(const int16_t *pcm16, int64_t sample_count, int channels, int frequency)
{
    ALuint new_handle = gen_buffer();

    if (new_handle == 0)
        return 0;

    // Clear any error left over from an earlier AL call - alGetError() is a single sticky flag, not a queue, so a stale error here would be misattributed to alBufferData below.
    alGetError_();

    alBufferData_(new_handle, channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, pcm16, (ALsizei)(sample_count * sizeof(int16_t)), (ALsizei)frequency);

    ALenum error = alGetError_();

    if (error != AL_NO_ERROR)
    {
        VA_BACKEND_ERROR("Failed to upload decoded audio to OpenAL: ", ALErrorToString(error), " (", (int)(sample_count / channels), " frames at ", frequency, " Hz)");
        delete_buffer(new_handle);
        return 0;
    }

    return new_handle;
}

void OpenALBackend::delete_sound_buffer(ALuint handle)
{
    if (handle == 0 || !singleton)
        return;

    singleton->alGetError_();
    singleton->delete_buffer(handle);

    // Fails if a source still has this buffer attached, which leaks it
    ALenum error = singleton->alGetError_();

    if (error != AL_NO_ERROR)
        VA_BACKEND_ERROR("alDeleteBuffers failed: ", ALErrorToString(error));
}

OpenALBackend::~OpenALBackend()
{
    shutdown();
}

ALenum OpenALBackend::to_al(VADistanceModel model)
{
    switch (model)
    {
        case VADistanceModel::None: return AL_NONE;
        case VADistanceModel::Inverse: return AL_INVERSE_DISTANCE;
        case VADistanceModel::Linear: return AL_LINEAR_DISTANCE;
        case VADistanceModel::LinearClamped: return AL_LINEAR_DISTANCE_CLAMPED;
        case VADistanceModel::Exponent: return AL_EXPONENT_DISTANCE;
        case VADistanceModel::ExponentClamped: return AL_EXPONENT_DISTANCE_CLAMPED;
        default: return AL_INVERSE_DISTANCE_CLAMPED;
    }
}

ALenum OpenALBackend::to_al(VAPcmFormat format)
{
    switch (format)
    {
        case VAPcmFormat::Mono8: return AL_FORMAT_MONO8;
        case VAPcmFormat::Mono16: return AL_FORMAT_MONO16;
        case VAPcmFormat::Stereo8: return AL_FORMAT_STEREO8;
        case VAPcmFormat::Stereo16: return AL_FORMAT_STEREO16;
        case VAPcmFormat::MonoFloat: return AL_FORMAT_MONO_FLOAT32;
        default: return AL_FORMAT_STEREO_FLOAT32;
    }
}

void OpenALBackend::apply_master_volume(float volume)
{
    alListenerf_(AL_GAIN, volume);
}

void OpenALBackend::apply_distance_model(VADistanceModel model)
{
    alDistanceModel_(to_al(model));
}

void OpenALBackend::apply_meters_per_unit(float value)
{
    alListenerf_(AL_METERS_PER_UNIT, value);
}

void OpenALBackend::apply_speed_of_sound(float value)
{
    alSpeedOfSound_(value);
}

void OpenALBackend::apply_doppler_scale(float scale)
{
    alDopplerFactor_(scale);
}

void OpenALBackend::apply_listener_position(const Vector3 &position)
{
    ALfloat values[3] = {position.x, position.y, position.z};
    alListenerfv_(AL_POSITION, values);
}

void OpenALBackend::apply_listener_velocity(const Vector3 &velocity)
{
    ALfloat values[3] = {velocity.x, velocity.y, velocity.z};
    alListenerfv_(AL_VELOCITY, values);
}

void OpenALBackend::apply_listener_orientation(const Vector3 &forward, const Vector3 &up)
{
    ALfloat values[6] = {forward.x, forward.y, forward.z, up.x, up.y, up.z};
    alListenerfv_(AL_ORIENTATION, values);
}

// The OpenAL Soft shared library SConstruct copies next to this GDExtension binary, per platform.
#ifdef _WIN32
static const char *OPENAL_LIBRARY_NAME = "soft_oal.dll";
#elif defined(__APPLE__)
static const char *OPENAL_LIBRARY_NAME = "libopenal.1.dylib";
#else
static const char *OPENAL_LIBRARY_NAME = "libopenal.so.1";
#endif

bool OpenALBackend::load_library()
{
    // Loading soft_oal.dll or libopenal.so.1 by name only searches the OS's default paths, not this GDExtension's folder
    std::string full_path = va_get_own_module_directory() + OPENAL_LIBRARY_NAME;

    library = va_open_library(full_path.c_str());

    // Fall back to the OS's default search path if the binary isn't next in this GDExtension's folder
    if (!library)
        library = va_open_library(OPENAL_LIBRARY_NAME);

    if (!library)
    {
        VA_BACKEND_ERROR("Failed to load ", OPENAL_LIBRARY_NAME, " from ", String::utf8(full_path.c_str()));
        return false;
    }

    return true;
}

template <typename T>
static bool resolve(va_dynamic_library library, const char *name, T &out_fn)
{
    out_fn = reinterpret_cast<T>(va_library_symbol(library, name));

    if (!out_fn)
    {
        VA_BACKEND_ERROR(OPENAL_LIBRARY_NAME, " is missing expected export: ", name);
        return false;
    }

    return true;
}

// EFX entry points aren't exports of soft_oal.dll (they're an AL extension) - must be looked up via alGetProcAddress instead of Win32 GetProcAddress.
template <typename T>
static bool resolve_ext(alGetProcAddressFn alGetProcAddress, const char *name, T &out_fn)
{
    out_fn = reinterpret_cast<T>(alGetProcAddress(name));

    if (!out_fn)
    {
        VA_BACKEND_ERROR("OpenAL Soft is missing expected EFX entry point: ", name);
        return false;
    }

    return true;
}

bool OpenALBackend::resolve_functions()
{
    bool ok = true;

    ok &= resolve(library, "alcOpenDevice", alcOpenDevice_);
    ok &= resolve(library, "alcCloseDevice", alcCloseDevice_);
    ok &= resolve(library, "alcCreateContext", alcCreateContext_);
    ok &= resolve(library, "alcMakeContextCurrent", alcMakeContextCurrent_);
    ok &= resolve(library, "alcDestroyContext", alcDestroyContext_);
    ok &= resolve(library, "alcGetError", alcGetError_);
    ok &= resolve(library, "alcGetString", alcGetString_);
    ok &= resolve(library, "alcGetIntegerv", alcGetIntegerv_);
    ok &= resolve(library, "alcIsExtensionPresent", alcIsExtensionPresent_);
    ok &= resolve(library, "alcGetProcAddress", alcGetProcAddress_);

    ok &= resolve(library, "alcCaptureOpenDevice", alcCaptureOpenDevice_);
    ok &= resolve(library, "alcCaptureCloseDevice", alcCaptureCloseDevice_);
    ok &= resolve(library, "alcCaptureStart", alcCaptureStart_);
    ok &= resolve(library, "alcCaptureStop", alcCaptureStop_);
    ok &= resolve(library, "alcCaptureSamples", alcCaptureSamples_);

    ok &= resolve(library, "alGetError", alGetError_);
    ok &= resolve(library, "alGetString", alGetString_);
    ok &= resolve(library, "alDistanceModel", alDistanceModel_);
    ok &= resolve(library, "alSpeedOfSound", alSpeedOfSound_);
    ok &= resolve(library, "alDopplerFactor", alDopplerFactor_);
    ok &= resolve(library, "alListenerf", alListenerf_);
    ok &= resolve(library, "alListenerfv", alListenerfv_);

    ok &= resolve(library, "alGenBuffers", alGenBuffers_);
    ok &= resolve(library, "alDeleteBuffers", alDeleteBuffers_);
    ok &= resolve(library, "alIsBuffer", alIsBuffer_);
    ok &= resolve(library, "alGetBufferi", alGetBufferi_);
    ok &= resolve(library, "alBufferData", alBufferData_);

    ok &= resolve(library, "alGenSources", alGenSources_);
    ok &= resolve(library, "alDeleteSources", alDeleteSources_);
    ok &= resolve(library, "alIsSource", alIsSource_);
    ok &= resolve(library, "alSourcei", alSourcei_);
    ok &= resolve(library, "alSourcef", alSourcef_);
    ok &= resolve(library, "alSourcefv", alSourcefv_);
    ok &= resolve(library, "alGetSourcei", alGetSourcei_);
    ok &= resolve(library, "alGetSourcef", alGetSourcef_);
    ok &= resolve(library, "alSourcePlay", alSourcePlay_);
    ok &= resolve(library, "alSourceStop", alSourceStop_);
    ok &= resolve(library, "alSourcePause", alSourcePause_);
    ok &= resolve(library, "alSource3i", alSource3i_);

    ok &= resolve(library, "alGetProcAddress", alGetProcAddress_);

    return ok;
}

// EFX function pointers aren't core soft_oal.dll exports - resolved via alGetProcAddress only after a device/context exist. Missing EFX isn't a hard
// failure: has_efx() just stays false and callers (ALFilter) are expected to no-op.
bool OpenALBackend::resolve_efx_functions()
{
    if (alcIsExtensionPresent_(device, ALC_EXT_EFX_NAME) == ALC_FALSE)
    {
        VA_WARN("ALC_EXT_EFX not present on this device - lowpass filter support disabled");
        return false;
    }

    bool ok = true;

    ok &= resolve_ext(alGetProcAddress_, "alGenFilters", alGenFilters_);
    ok &= resolve_ext(alGetProcAddress_, "alDeleteFilters", alDeleteFilters_);
    ok &= resolve_ext(alGetProcAddress_, "alFilteri", alFilteri_);
    ok &= resolve_ext(alGetProcAddress_, "alFilterf", alFilterf_);

    ok &= resolve_ext(alGetProcAddress_, "alGenEffects", alGenEffects_);
    ok &= resolve_ext(alGetProcAddress_, "alDeleteEffects", alDeleteEffects_);
    ok &= resolve_ext(alGetProcAddress_, "alEffecti", alEffecti_);
    ok &= resolve_ext(alGetProcAddress_, "alEffectf", alEffectf_);
    ok &= resolve_ext(alGetProcAddress_, "alEffectfv", alEffectfv_);

    ok &= resolve_ext(alGetProcAddress_, "alGenAuxiliaryEffectSlots", alGenAuxiliaryEffectSlots_);
    ok &= resolve_ext(alGetProcAddress_, "alDeleteAuxiliaryEffectSlots", alDeleteAuxiliaryEffectSlots_);
    ok &= resolve_ext(alGetProcAddress_, "alAuxiliaryEffectSloti", alAuxiliaryEffectSloti_);
    ok &= resolve_ext(alGetProcAddress_, "alAuxiliaryEffectSlotf", alAuxiliaryEffectSlotf_);

    // AL_SOFT_callback_buffer is a separate AL extension, resolved here too since both need a device/context. Not folded into `ok`: a driver
    // without this should still get EFX, it just won't get streaming buffers (ALStreamBuffer checks al_buffer_callback_soft() for null).
    if (!resolve_ext(alGetProcAddress_, "alBufferCallbackSOFT", alBufferCallbackSOFT_))
    {
        VA_WARN("AL_SOFT_callback_buffer not present on this device - streaming source support disabled");
    }

    return ok;
}

void OpenALBackend::unload_library()
{
    if (library)
    {
        va_close_library(library);
        library = nullptr;
    }
}

static void set_environment_variable(const char *name, const String &value)
{
#ifdef _WIN32
    CharWideString wide_name = String(name).wide_string();
    CharWideString wide_value = value.wide_string();

    SetEnvironmentVariableW(wide_name.get_data(), wide_value.get_data());

    // soft_oal.dll reads the environment through the shared UCRT's _wgetenv, which keeps its own copy that SetEnvironmentVariableW doesn't update (and this module's static CRT has a separate copy again)
    using wputenv_s_fn = errno_t(__cdecl *)(const wchar_t *, const wchar_t *);

    if (HMODULE ucrt = LoadLibraryA("ucrtbase.dll"))
    {
        if (wputenv_s_fn wputenv_s = reinterpret_cast<wputenv_s_fn>(GetProcAddress(ucrt, "_wputenv_s")))
            wputenv_s(wide_name.get_data(), wide_value.get_data());
    }
#else
    setenv(name, value.utf8().get_data(), 1);
#endif
}

// OpenAL Soft searches ALSOFT_LOCAL_PATH for .mhr files every time it enumerates HRTFs, so pointing it at the project's HRTF folder makes those files show up alongside its built-in HRTF
void OpenALBackend::prepare_hrtf_folder()
{
    OS *os = OS::get_singleton();

    // Don't override a path the user set themselves
    if (os->has_environment("ALSOFT_LOCAL_PATH"))
        return;

    String folder = ProjectSettings::get_singleton()->get_setting("audio/vaudio/openal_hrtf_folder");

    if (folder.is_empty())
        return;

    // Outside of exported builds the folder is a real directory, so OpenAL Soft can search it directly - even if it's created or filled after startup
    if (!os->has_feature("template"))
    {
        set_environment_variable("ALSOFT_LOCAL_PATH", ProjectSettings::get_singleton()->globalize_path(folder));
        return;
    }

    if (!DirAccess::dir_exists_absolute(folder))
        return;

    PackedStringArray mhr_files;
    PackedStringArray files = DirAccess::get_files_at(folder);

    for (int i = 0; i < files.size(); i++)
    {
        if (files[i].get_extension().to_lower() == "mhr")
            mhr_files.push_back(files[i]);
    }

    if (mhr_files.is_empty())
        return;

    // OpenAL Soft can't open files inside an exported .pck, so copy them out to a real folder first
    String local_path = "user://vaudio/hrtf";
    DirAccess::make_dir_recursive_absolute(local_path);

    // Clear out HRTFs left behind by older builds of this game
    PackedStringArray old_files = DirAccess::get_files_at(local_path);

    for (int i = 0; i < old_files.size(); i++)
        DirAccess::remove_absolute(local_path.path_join(old_files[i]));

    for (int i = 0; i < mhr_files.size(); i++)
    {
        if (DirAccess::copy_absolute(folder.path_join(mhr_files[i]), local_path.path_join(mhr_files[i])) != OK)
            VA_WARN("Failed to copy HRTF file '", mhr_files[i], "' to ", local_path);
    }

    set_environment_variable("ALSOFT_LOCAL_PATH", ProjectSettings::get_singleton()->globalize_path(local_path));
}

void OpenALBackend::build_attributes(ALCint *attributes)
{
    int count = 0;

    if (max_auxiliary_sends > 0)
    {
        attributes[count++] = ALC_MAX_AUXILIARY_SENDS;
        attributes[count++] = max_auxiliary_sends;
    }

    if (max_mono_sources > 0)
    {
        attributes[count++] = ALC_MONO_SOURCES;
        attributes[count++] = max_mono_sources;
    }

    if (max_stereo_sources > 0)
    {
        attributes[count++] = ALC_STEREO_SOURCES;
        attributes[count++] = max_stereo_sources;
    }

    if (sample_rate > 0)
    {
        attributes[count++] = ALC_FREQUENCY;
        attributes[count++] = sample_rate;
    }

    attributes[count++] = ALC_HRTF_SOFT;
    attributes[count++] = hrtf_enabled ? ALC_TRUE : ALC_FALSE;

    int hrtf_index = find_hrtf_index();

    if (hrtf_index >= 0)
    {
        attributes[count++] = ALC_HRTF_ID_SOFT;
        attributes[count++] = hrtf_index;
    }

    attributes[count++] = 0;
}

int OpenALBackend::find_hrtf_index()
{
    if (hrtf_name.empty() || !device || !alcGetStringiSOFT_)
        return -1;

    // Querying the count re-enumerates OpenAL Soft's HRTF list, so these indices match what ALC_HRTF_ID_SOFT is resolved against
    ALCint count = 0;
    alcGetIntegerv_(device, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &count);

    for (int i = 0; i < count; i++)
    {
        const ALCchar *name = alcGetStringiSOFT_(device, ALC_HRTF_SPECIFIER_SOFT, i);

        if (name && hrtf_name == name)
            return i;
    }

    VA_WARN("HRTF '", String::utf8(hrtf_name.c_str()), "' was not found, using the default HRTF instead");
    return -1;
}

bool OpenALBackend::open_device_and_context()
{
    const ALCchar *requested_device = device_name.empty() ? nullptr : device_name.c_str();
    device = alcOpenDevice_(requested_device);

    if (!device)
    {
        VA_BACKEND_ERROR("alcOpenDevice failed for device '", String::utf8(device_name.c_str()), "'");
        return false;
    }

    // Resolved before the context is created, since build_attributes() needs alcGetStringiSOFT to map hrtf_name to an ALC_HRTF_ID_SOFT index
    if (alcIsExtensionPresent_(device, "ALC_SOFT_HRTF") == ALC_FALSE)
    {
        alcGetStringiSOFT_ = nullptr;
        alcResetDeviceSOFT_ = nullptr;
    }
    else
    {
        alcGetStringiSOFT_ = reinterpret_cast<alcGetStringiSOFTFn>(alcGetProcAddress_(device, "alcGetStringiSOFT"));
        alcResetDeviceSOFT_ = reinterpret_cast<alcResetDeviceSOFTFn>(alcGetProcAddress_(device, "alcResetDeviceSOFT"));
    }

    ALCint attributes[13];
    build_attributes(attributes);

    context = alcCreateContext_(device, attributes);

    if (!context)
    {
        VA_BACKEND_ERROR("alcCreateContext failed, ALC error ", (int)alcGetError_(device));
        alcCloseDevice_(device);
        device = nullptr;
        return false;
    }

    if (alcMakeContextCurrent_(context) == ALC_FALSE)
    {
        VA_BACKEND_ERROR("alcMakeContextCurrent failed, ALC error ", (int)alcGetError_(device));
        alcDestroyContext_(context);
        context = nullptr;
        alcCloseDevice_(device);
        device = nullptr;
        return false;
    }

    // The distance model, listener and doppler settings are per-context state, lost whenever open_device_and_context() recreates the context
    apply_all_settings();

    efx_present = resolve_efx_functions();

    // Per-device state like EFX - re-queried every (re)open since a fallback device reached after switching outputs might not support it.
    if (alcIsExtensionPresent_(device, "ALC_SOFT_reopen_device") == ALC_FALSE)
    {
        alcReopenDeviceSOFT_ = nullptr;
    }
    else
    {
        alcReopenDeviceSOFT_ = reinterpret_cast<alcReopenDeviceSOFTFn>(alcGetProcAddress_(device, "alcReopenDeviceSOFT"));
    }

    return true;
}

// Reads device_name/max_auxiliary_sends/max_mono_sources/max_stereo_sources/sample_rate/hrtf_enabled from the audio/vaudio/* Project Settings
// registered by register_project_settings() (register_types_common.cpp), which must already have run so every setting exists with its registered default.
void OpenALBackend::read_settings_from_project_settings()
{
    ProjectSettings *settings = ProjectSettings::get_singleton();

    // PROPERTY_HINT_ENUM on a STRING property stores the literal entry text picked (see VAAudioBackend::refresh_device_lists()), so the
    // UI writes DEFAULT_DEVICE_LABEL when a user picks "System Default" - translate that back to "" (alcOpenDevice's own "use the driver default").
    String device_name_setting = settings->get_setting("audio/vaudio/output_device");

    if (device_name_setting == String(DEFAULT_DEVICE_LABEL))
        device_name_setting = String();

    CharString device_name_utf8 = device_name_setting.utf8();
    device_name = device_name_utf8.get_data();

    max_auxiliary_sends = MAX(1, (int)settings->get_setting("audio/vaudio/openal_max_reverb_sends"));
    max_mono_sources = settings->get_setting("audio/vaudio/openal_max_mono_sources");
    max_stereo_sources = settings->get_setting("audio/vaudio/openal_max_stereo_sources");
    sample_rate = settings->get_setting("audio/vaudio/sample_rate");
    hrtf_enabled = settings->get_setting("audio/vaudio/openal_hrtf_enabled");

    // Same DEFAULT_*_LABEL translation as output_device above
    String hrtf_setting = settings->get_setting("audio/vaudio/openal_hrtf");

    if (hrtf_setting == String(DEFAULT_HRTF_LABEL))
        hrtf_setting = String();

    hrtf_name = hrtf_setting.utf8().get_data();
}

bool OpenALBackend::initialise()
{
    if (is_initialised())
    {
        return true;
    }

    read_settings_from_project_settings();

    prepare_hrtf_folder();

    if (!load_library())
    {
        return false;
    }

    if (!resolve_functions())
    {
        unload_library();
        return false;
    }

    if (!open_device_and_context())
    {
        unload_library();
        return false;
    }

    VA_LOG("OpenAL initialised - device '", device_name.empty() ? "(default)" : device_name.c_str(),
        "', max_reverb_sends ", max_auxiliary_sends, ", max_mono_sources ", max_mono_sources,
        ", max_stereo_sources ", max_stereo_sources, ", sample_rate ", sample_rate, ", hrtf_enabled ", hrtf_enabled,
        ", hrtf '", get_active_hrtf(), "'");

    singleton = this;
    return true;
}

void OpenALBackend::close_device()
{
    if (singleton == this)
    {
        singleton = nullptr;
    }

    if (context)
    {
        alcMakeContextCurrent_(nullptr);
        alcDestroyContext_(context);
        context = nullptr;
    }

    if (device)
    {
        alcCloseDevice_(device);
        device = nullptr;
    }
}

bool OpenALBackend::recreate_device()
{
    raise_device_destroying();
    close_device();

    if (!open_device_and_context())
        return false;

    singleton = this;
    raise_device_recreated();
    return true;
}

bool OpenALBackend::set_output_device(const String &name)
{
    return reinitialize(name == String(DEFAULT_DEVICE_LABEL) ? String() : name);
}

bool OpenALBackend::reinitialize(const String &new_device_name)
{
    CharString new_device_name_utf8 = new_device_name.utf8();
    const char *new_device_name_cstr = new_device_name_utf8.get_data();

    if (device_name == new_device_name_cstr && is_initialised())
    {
        return true;
    }

    device_name = new_device_name_cstr;

    if (!library)
    {
        return initialise();
    }

    // Prefer alcReopenDeviceSOFT when supported: it redirects the existing ALC device/context to the new output device in place, keeping every
    // existing AL object valid - unlike the close_device()+open_device_and_context() fallback below, a full teardown/recreate that invalidates them.
    if (alcReopenDeviceSOFT_ && is_initialised())
    {
        const ALCchar *requested_device = device_name.empty() ? nullptr : device_name.c_str();

        ALCint attributes[13];
        build_attributes(attributes);

        if (alcReopenDeviceSOFT_(device, requested_device, attributes) == ALC_TRUE)
        {
            singleton = this;
            return true;
        }

        VA_WARN("alcReopenDeviceSOFT failed for device '", String::utf8(device_name.c_str()),
            "', ALC error ", (int)alcGetError_(device), " - falling back to full device recreation");
    }

    return recreate_device();
}

PackedStringArray OpenALBackend::get_output_devices()
{
    PackedStringArray devices;

    if (!alcGetString_)
    {
        return devices;
    }

    ALCenum specifier = alcIsExtensionPresent_ && alcIsExtensionPresent_(nullptr, "ALC_ENUMERATE_ALL_EXT")
        ? ALC_ALL_DEVICES_SPECIFIER
        : ALC_DEVICE_SPECIFIER;

    // Standard ALC enumeration convention: a single buffer of null-terminated device names, itself terminated by an extra empty string.
    const ALCchar *list = alcGetString_(nullptr, specifier);

    if (!list)
    {
        return devices;
    }

    while (*list)
    {
        devices.push_back(String::utf8(list));
        list += strlen(list) + 1;
    }

    return devices;
}

PackedStringArray OpenALBackend::get_capture_devices()
{
    PackedStringArray devices;

    if (!alcGetString_)
    {
        return devices;
    }

    // Same convention as get_output_devices() above, just for ALC_CAPTURE_DEVICE_SPECIFIER.
    const ALCchar *list = alcGetString_(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER);

    if (!list)
    {
        return devices;
    }

    while (*list)
    {
        devices.push_back(String::utf8(list));
        list += strlen(list) + 1;
    }

    return devices;
}

PackedStringArray OpenALBackend::get_available_hrtfs()
{
    PackedStringArray hrtfs;

    if (!device || !alcGetStringiSOFT_)
        return hrtfs;

    ALCint count = 0;
    alcGetIntegerv_(device, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &count);

    for (int i = 0; i < count; i++)
    {
        if (const ALCchar *name = alcGetStringiSOFT_(device, ALC_HRTF_SPECIFIER_SOFT, i))
            hrtfs.push_back(String::utf8(name));
    }

    return hrtfs;
}

String OpenALBackend::get_active_hrtf()
{
    if (!device || !alcGetStringiSOFT_)
        return String();

    ALCint status = ALC_HRTF_DISABLED_SOFT;
    alcGetIntegerv_(device, ALC_HRTF_STATUS_SOFT, 1, &status);

    if (status != ALC_HRTF_ENABLED_SOFT && status != ALC_HRTF_REQUIRED_SOFT)
        return String();

    const ALCchar *name = alcGetString_(device, ALC_HRTF_SPECIFIER_SOFT);
    return name ? String::utf8(name) : String();
}

bool OpenALBackend::set_hrtf(const String &name)
{
    std::string new_name = name == String(DEFAULT_HRTF_LABEL) ? std::string() : std::string(name.utf8().get_data());

    if (new_name == hrtf_name && is_initialised())
        return true;

    hrtf_name = new_name;
    return reset_device();
}

bool OpenALBackend::set_hrtf_enabled(bool value)
{
    if (value == hrtf_enabled && is_initialised())
        return true;

    hrtf_enabled = value;
    return reset_device();
}

// Applies the current attributes (HRTF etc.) to the open device. alcResetDeviceSOFT keeps every AL object valid, the full recreation fallback doesn't
bool OpenALBackend::reset_device()
{
    if (!library)
        return false;

    if (alcResetDeviceSOFT_ && is_initialised())
    {
        ALCint attributes[13];
        build_attributes(attributes);

        if (alcResetDeviceSOFT_(device, attributes) == ALC_TRUE)
            return true;

        VA_WARN("Failed to reset the OpenAL device (ALC error ", (int)alcGetError_(device), ") - falling back to full device recreation");
    }

    return recreate_device();
}

// Once the output device is lost (e.g. headphones unplugged), OpenAL Soft stops every source and new ones stop as soon as they play, so reopen it in place, which keeps every AL object valid. Throttled, since every VAWorld calls it each frame
void OpenALBackend::check_device_connected()
{
    if (!is_initialised())
        return;

    uint64_t now = Time::get_singleton()->get_ticks_msec();
    if (now < next_connection_check_ms)
        return;

    next_connection_check_ms = now + 1000;

    if (alcIsExtensionPresent_(device, "ALC_EXT_disconnect") == ALC_FALSE)
        return;

    ALCint connected = ALC_TRUE;
    alcGetIntegerv_(device, ALC_CONNECTED, 1, &connected);

    if (connected)
    {
        device_lost = false;
        return;
    }

    if (!device_lost)
        VA_WARN("The audio output device was disconnected, reopening it");

    ALCint attributes[13];
    build_attributes(attributes);

    if (alcReopenDeviceSOFT_ && alcReopenDeviceSOFT_(device, device_name.empty() ? nullptr : device_name.c_str(), attributes) == ALC_TRUE)
    {
        device_lost = false;
        return;
    }

    if (!device_lost)
        VA_BACKEND_ERROR("Failed to reopen the audio output device after it was disconnected, retrying every second");

    device_lost = true;
}

void OpenALBackend::update()
{
    check_device_connected();
}

void OpenALBackend::shutdown()
{
    if (is_initialised())
        raise_device_destroying();

    close_device();
    unload_library();
}

void OpenALBackend::refresh_setting_hints()
{
    ProjectSettings *settings = ProjectSettings::get_singleton();

    PackedStringArray hrtfs = get_available_hrtfs();
    hrtfs.insert(0, DEFAULT_HRTF_LABEL);

    // Keep a selected HRTF that isn't installed on this machine in the list, rather than the dropdown silently showing a different entry
    String current = settings->get_setting("audio/vaudio/openal_hrtf");

    if (!hrtfs.has(current))
        hrtfs.push_back(current);

    Dictionary hrtf_info;
    hrtf_info["name"] = "audio/vaudio/openal_hrtf";
    hrtf_info["type"] = Variant::STRING;
    hrtf_info["hint"] = PROPERTY_HINT_ENUM;
    hrtf_info["hint_string"] = String(",").join(hrtfs);
    settings->add_property_info(hrtf_info);
}

std::unique_ptr<VASound> OpenALBackend::create_sound(const int16_t *pcm16, int64_t sample_count, int channels, int frequency)
{
    if (!is_initialised())
        return nullptr;

    ALuint buffer = upload_pcm16(pcm16, sample_count, channels, frequency);

    if (buffer == 0)
        return nullptr;

    return std::make_unique<OpenALSound>(buffer);
}

std::unique_ptr<VAStream> OpenALBackend::create_stream(VAPcmFormat format, int frequency)
{
    auto stream = std::make_unique<OpenALStream>();
    return stream->create(format, frequency) ? std::move(stream) : nullptr;
}

std::unique_ptr<VAReverbEffect> OpenALBackend::create_reverb_effect()
{
    return std::make_unique<OpenALReverbEffect>();
}

std::unique_ptr<VACaptureDevice> OpenALBackend::open_capture_device(const String &device_name, VAPcmFormat format, int frequency, int buffer_frames, VACaptureCallback on_data)
{
    auto capture = std::make_unique<OpenALCaptureDevice>();
    return capture->open(device_name, format, frequency, buffer_frames, std::move(on_data)) ? std::move(capture) : nullptr;
}
