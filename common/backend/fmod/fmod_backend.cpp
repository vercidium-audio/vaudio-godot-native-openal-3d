// AI-created

#include "fmod_backend.h"

#include <godot_cpp/classes/project_settings.hpp>

#include "backend/va_audio_backend_registry.h"
#include "fmod_library.h"
#include "fmod_objects.h"
#include "va_device_name.h"
#include "va_engine_util.h"

#include <algorithm>

using namespace godot;

// Registers with the backend registry when the plugin library loads, so core code never names FMODBackend
static const bool registered = VAAudioBackendRegistry::register_backend(FMODBackend::BACKEND_NAME, []() -> VAAudioBackendBase * { return new FMODBackend(); });

static FMODBackend *singleton = nullptr;

// FMOD's doppler uses a fixed speed of sound, in metres per second
static constexpr float FMOD_SPEED_OF_SOUND = 340.0f;

FMODBackend *FMODBackend::get_singleton()
{
    return singleton;
}

bool FMODBackend::check(FMOD_RESULT result, const char *call)
{
    if (result == FMOD_OK)
        return true;

    VA_BACKEND_ERROR("FMOD ", call, " failed: ", FMOD_ErrorString(result), " (", (int)result, ")");
    return false;
}

FMOD_SOUND_FORMAT FMODBackend::to_fmod(VAPcmFormat format)
{
    switch (va_pcm_bytes_per_sample(format))
    {
        case 1: return FMOD_SOUND_FORMAT_PCM8;
        case 2: return FMOD_SOUND_FORMAT_PCM16;
        default: return FMOD_SOUND_FORMAT_PCMFLOAT;
    }
}

void FMODBackend::toggle_sign8(uint8_t *data, unsigned int length)
{
    for (unsigned int i = 0; i < length; i++)
        data[i] ^= 0x80;
}

static String format_version(unsigned int version)
{
    return String::num_int64(version >> 16, 16) + "." + String::num_int64((version >> 8) & 0xff, 16).pad_zeros(2) + "." + String::num_int64(version & 0xff, 16).pad_zeros(2);
}

FMODBackend::~FMODBackend()
{
    shutdown();
}

void FMODBackend::ensure_settings_read()
{
    if (settings_read)
        return;

    settings_read = true;

    ProjectSettings *settings = ProjectSettings::get_singleton();

    String device_setting = settings->get_setting("audio/vaudio/output_device", "");
    output_device_name = device_setting == String(DEFAULT_DEVICE_LABEL) ? String() : device_setting;

    sample_rate = settings->get_setting("audio/vaudio/sample_rate", 0);
    library_path = settings->get_setting("audio/vaudio/fmod_library_path", "");
    max_channels = std::clamp((int)settings->get_setting("audio/vaudio/fmod_max_channels", 512), 1, 4095);
    dsp_buffer_size = settings->get_setting("audio/vaudio/fmod_dsp_buffer_size", 0);
}

bool FMODBackend::load_library()
{
    if (library_loaded)
        return true;

    String error;
    String loaded_from = FMODLibrary::load(library_path, error);

    if (loaded_from.is_empty())
    {
        VA_BACKEND_ERROR(error);
        return false;
    }

    VA_LOG("Loaded FMOD from ", loaded_from);
    library_loaded = true;
    return true;
}

bool FMODBackend::initialise()
{
    if (is_initialised() || initialise_failed)
        return is_initialised();

    ensure_settings_read();

    if (!load_library() || !create_system())
    {
        initialise_failed = true;
        return false;
    }

    singleton = this;
    return true;
}

bool FMODBackend::create_system()
{
    FMOD_SYSTEM *new_system = nullptr;
    FMOD_RESULT result = va_fmod.FMOD_System_Create(&new_system, FMOD_VERSION);

    if (result == FMOD_ERR_HEADER_MISMATCH)
    {
        VA_BACKEND_ERROR("The FMOD library's version doesn't match the FMOD ", format_version(FMOD_VERSION), " API vaudio was built with. Use an FMOD Engine 2.03.x library");
        return false;
    }

    if (!check(result, "System_Create"))
        return false;

    if (sample_rate > 0)
        check(va_fmod.FMOD_System_SetSoftwareFormat(new_system, sample_rate, FMOD_SPEAKERMODE_DEFAULT, 0), "System::setSoftwareFormat");

    // FMOD's default is 4 buffers of 1024 samples
    if (dsp_buffer_size > 0)
        check(va_fmod.FMOD_System_SetDSPBufferSize(new_system, (unsigned int)dsp_buffer_size, 4), "System::setDSPBufferSize");

    select_driver(new_system);

    // Godot is right-handed, FMOD defaults to left-handed
    if (!check(va_fmod.FMOD_System_Init(new_system, max_channels, FMOD_INIT_3D_RIGHTHANDED, nullptr), "System::init") || !check(va_fmod.FMOD_System_GetMasterChannelGroup(new_system, &master_group), "System::getMasterChannelGroup"))
    {
        va_fmod.FMOD_System_Release(new_system);
        master_group = nullptr;
        return false;
    }

    system = new_system;

    unsigned int version = 0;
    check(va_fmod.FMOD_System_GetVersion(system, &version, nullptr), "System::getVersion");
    VA_LOG("FMOD ", format_version(version), " initialised");

    apply_all_settings();
    return true;
}

void FMODBackend::update()
{
    if (is_initialised())
        check(va_fmod.FMOD_System_Update(system), "System::update");
}

void FMODBackend::shutdown()
{
    if (!is_initialised())
        return;

    raise_device_destroying();

    // Nodes can outlive the backend, so their voices are detached here and do nothing once deleted
    std::vector<FMODVoice *> remaining(voices.begin(), voices.end());

    for (FMODVoice *voice : remaining)
        voice->release();

    if (singleton == this)
        singleton = nullptr;

    // Also frees every sound still held by a node
    check(va_fmod.FMOD_System_Release(system), "System::release");
    system = nullptr;
    master_group = nullptr;
}

void FMODBackend::apply_master_volume(float volume)
{
    check(va_fmod.FMOD_ChannelGroup_SetVolume(master_group, volume), "ChannelGroup::setVolume");
}

// FMOD has no global distance model, so each voice samples it into its own rolloff curve
void FMODBackend::apply_distance_model(VADistanceModel model)
{
    for (FMODVoice *voice : voices)
        voice->apply_rolloff();
}

void FMODBackend::apply_meters_per_unit(float value)
{
    apply_3d_settings();
}

void FMODBackend::apply_speed_of_sound(float value)
{
    apply_3d_settings();
}

void FMODBackend::apply_doppler_scale(float scale)
{
    apply_3d_settings();
}

// FMOD has no speed of sound setting, so it's folded into the doppler scale: scaling every velocity by FMOD_SPEED_OF_SOUND / speed_of_sound gives the same pitch shift as changing the speed of sound
void FMODBackend::apply_3d_settings()
{
    float doppler_scale = get_doppler_scale() * FMOD_SPEED_OF_SOUND / std::max(0.0001f, get_speed_of_sound());
    float units_per_metre = 1.0f / std::max(0.0001f, get_meters_per_unit());

    check(va_fmod.FMOD_System_Set3DSettings(system, doppler_scale, units_per_metre, 1.0f), "System::set3DSettings");
}

void FMODBackend::apply_listener_position(const Vector3 &position)
{
    listener_position = to_fmod(position);
    apply_listener();
}

void FMODBackend::apply_listener_velocity(const Vector3 &velocity)
{
    listener_velocity = to_fmod(velocity);
    apply_listener();
}

void FMODBackend::apply_listener_orientation(const Vector3 &forward, const Vector3 &up)
{
    listener_forward = to_fmod(forward);
    listener_up = to_fmod(up);
    apply_listener();
}

void FMODBackend::apply_listener()
{
    check(va_fmod.FMOD_System_Set3DListenerAttributes(system, 0, &listener_position, &listener_velocity, &listener_forward, &listener_up), "System::set3DListenerAttributes");
}

bool FMODBackend::set_output_device(const String &name)
{
    ensure_settings_read();
    output_device_name = name == String(DEFAULT_DEVICE_LABEL) ? String() : name;

    // FMOD moves every playing sound over to the new driver itself
    if (is_initialised())
        select_driver(system);

    return true;
}

void FMODBackend::select_driver(FMOD_SYSTEM *target)
{
    int index = 0;

    if (!output_device_name.is_empty())
    {
        index = get_driver_names(target).find(output_device_name);

        if (index < 0)
        {
            VA_WARN("The audio output device '", output_device_name, "' wasn't found, so the system default is used");
            index = 0;
        }
    }

    check(va_fmod.FMOD_System_SetDriver(target, index), "System::setDriver");
}

PackedStringArray FMODBackend::get_driver_names(FMOD_SYSTEM *target)
{
    PackedStringArray names;
    int count = 0;

    if (!check(va_fmod.FMOD_System_GetNumDrivers(target, &count), "System::getNumDrivers"))
        return names;

    for (int i = 0; i < count; i++)
    {
        char name[256] = {};
        check(va_fmod.FMOD_System_GetDriverInfo(target, i, name, sizeof(name), nullptr, nullptr, nullptr, nullptr), "System::getDriverInfo");
        names.push_back(String::utf8(name));
    }

    return names;
}

std::vector<FMODBackend::RecordDriver> FMODBackend::get_record_drivers(FMOD_SYSTEM *target)
{
    std::vector<RecordDriver> drivers;
    int count = 0;

    if (!check(va_fmod.FMOD_System_GetRecordNumDrivers(target, &count, nullptr), "System::getRecordNumDrivers"))
        return drivers;

    for (int i = 0; i < count; i++)
    {
        char name[256] = {};
        FMOD_DRIVER_STATE state = 0;

        if (check(va_fmod.FMOD_System_GetRecordDriverInfo(target, i, name, sizeof(name), nullptr, nullptr, nullptr, nullptr, &state), "System::getRecordDriverInfo"))
            drivers.push_back({i, String::utf8(name), (state & FMOD_DRIVER_STATE_CONNECTED) != 0, (state & FMOD_DRIVER_STATE_DEFAULT) != 0});
    }

    return drivers;
}

PackedStringArray FMODBackend::with_system(const std::function<PackedStringArray(FMOD_SYSTEM *)> &list, bool initialise_temporary)
{
    if (is_initialised())
        return list(system);

    ensure_settings_read();

    FMOD_SYSTEM *temporary = nullptr;

    if (!load_library() || !check(va_fmod.FMOD_System_Create(&temporary, FMOD_VERSION), "System_Create"))
        return PackedStringArray();

    PackedStringArray names;

    if (!initialise_temporary || check(va_fmod.FMOD_System_Init(temporary, 1, FMOD_INIT_NORMAL, nullptr), "System::init"))
        names = list(temporary);

    va_fmod.FMOD_System_Release(temporary);
    return names;
}

PackedStringArray FMODBackend::get_output_devices()
{
    return with_system([this](FMOD_SYSTEM *target) { return get_driver_names(target); }, false);
}

// VAInputStreamSource's device_name dropdown. Unplugged drivers are left out
PackedStringArray FMODBackend::get_capture_devices()
{
    return with_system([this](FMOD_SYSTEM *target)
    {
        PackedStringArray names;

        for (const RecordDriver &driver : get_record_drivers(target))
        {
            if (driver.connected)
                names.push_back(driver.name);
        }

        return names;
    }, true);
}

std::unique_ptr<VASound> FMODBackend::create_sound(const int16_t *pcm16, int64_t sample_count, int channels, int frequency)
{
    if (!is_initialised())
        return nullptr;

    FMOD_CREATESOUNDEXINFO exinfo = {};
    exinfo.cbsize = sizeof(exinfo);
    exinfo.length = (unsigned int)(sample_count * sizeof(int16_t));
    exinfo.numchannels = channels;
    exinfo.defaultfrequency = frequency;
    exinfo.format = FMOD_SOUND_FORMAT_PCM16;

    // FMOD copies the data. Each voice sets its own 2D/3D, relative and loop mode
    FMOD_MODE mode = FMOD_OPENMEMORY | FMOD_OPENRAW | FMOD_CREATESAMPLE | FMOD_LOOP_OFF | FMOD_3D;
    FMOD_SOUND *sound = nullptr;

    if (!check(va_fmod.FMOD_System_CreateSound(system, reinterpret_cast<const char *>(pcm16), mode, &exinfo, &sound), "System::createSound"))
        return nullptr;

    return std::make_unique<FMODSound>(sound, system, frequency, (float)(sample_count / channels) / (float)frequency);
}

std::unique_ptr<FMODVoice> FMODBackend::play_sound(FMOD_SOUND *sound, int sound_sample_rate, float length, bool spatialised, FMOD_SOUND *stream_sound)
{
    FMOD_CHANNEL *channel = nullptr;

    if (!is_initialised() || !check(va_fmod.FMOD_System_PlaySound(system, sound, nullptr, true, &channel), "System::playSound"))
        return nullptr;

    return std::make_unique<FMODVoice>(this, channel, (float)sound_sample_rate, length, spatialised, stream_sound);
}

std::unique_ptr<VAStream> FMODBackend::create_stream(VAPcmFormat format, int frequency)
{
    if (!is_initialised())
    {
        VA_BACKEND_ERROR("Can't open the stream because FMOD failed to start. Check the earlier vaudio errors in the Output panel");
        return nullptr;
    }

    auto stream = std::make_unique<FMODStream>();
    return stream->create(this, format, frequency) ? std::move(stream) : nullptr;
}

std::unique_ptr<VAReverbEffect> FMODBackend::create_reverb_effect()
{
    return nullptr;
}

std::unique_ptr<VACaptureDevice> FMODBackend::open_capture_device(const String &device_name, VAPcmFormat format, int frequency, int buffer_frames, VACaptureCallback on_data)
{
    VA_BACKEND_ERROR("Can't open the capture device because the native FMOD backend doesn't support capture yet");
    return nullptr;
}
