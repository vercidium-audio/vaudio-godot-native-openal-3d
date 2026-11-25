// AI-created

// Human-reviewed

#include "al_source_handle.h"

#include "../va_engine_util.h"
#include "al_manager.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

ALSourceHandle::~ALSourceHandle()
{
    destroy();
}

bool ALSourceHandle::create()
{
    ALManager *manager = ALManager::get_initialized_singleton("play sound");

    if (!manager)
        return false;

    // alGetError is a single sticky flag, so clear any stale error before reading the result of alGenSources below
    manager->al_get_error()();

    ALuint new_handle = 0;
    manager->al_gen_sources()(1, &new_handle);

    ALenum error = manager->al_get_error()();

    if (new_handle == 0 || error != AL_NO_ERROR)
    {
        // OpenAL Soft reports AL_OUT_OF_MEMORY when the device's source limit (mono + stereo sources) is reached
        if (error == AL_OUT_OF_MEMORY)
            VA_AL_ERROR("Can't play sound because too many sounds are playing at once. Increase audio/vaudio/max_stereo_sources in Project Settings. Error code: ", ALErrorToString(error));
        else
            VA_AL_ERROR("Failed to create an OpenAL source. Error code: ", ALErrorToString(error));

        return false;
    }

    handle = new_handle;

    // Godot's mix_audio always returns stereo frames, so every buffer is AL_FORMAT_STEREO16 (see ALBuffer::upload). OpenAL Soft leaves stereo sources unspatialized by default, so force it on
    manager->al_sourcei()(handle, AL_SOURCE_SPATIALIZE_SOFT, AL_TRUE);

    return true;
}

void ALSourceHandle::destroy()
{
    // Never created (create() failed) or already destroyed - the destructor always calls this, so it's expected
    if (handle == 0)
        return;

    ALManager *manager = ALManager::get_singleton();

    // No manager means OpenAL already shut down (plugin unloading, or a failed device switch), which destroyed every source along with its context
    if (manager && manager->is_initialized())
        manager->delete_source(handle);

    handle = 0;
}

void ALSourceHandle::set_buffer(ALuint buffer_handle)
{
    buffer = buffer_handle;
    ALManager::get_singleton()->al_sourcei()(handle, AL_BUFFER, (ALint)buffer_handle);
}

void ALSourceHandle::play()
{
    ALManager::get_singleton()->al_source_play()(handle);
}

void ALSourceHandle::stop()
{
    ALManager::get_singleton()->al_source_stop()(handle);
}

void ALSourceHandle::pause()
{
    ALManager::get_singleton()->al_source_pause()(handle);
}

ALint ALSourceHandle::get_state() const
{
    ALManager *manager = ALManager::get_singleton();

    if (!manager)
        return 0;

    ALint state = 0;
    manager->al_get_sourcei()(handle, AL_SOURCE_STATE, &state);
    return state;
}

void ALSourceHandle::set_seconds_offset(float seconds)
{
    ALManager::get_singleton()->al_sourcef()(handle, AL_SEC_OFFSET, seconds);
}

float ALSourceHandle::get_seconds_offset() const
{
    ALfloat seconds = 0;
    ALManager::get_singleton()->al_get_sourcef()(handle, AL_SEC_OFFSET, &seconds);
    return seconds;
}

float ALSourceHandle::get_buffer_length() const
{
    if (buffer == 0)
        return 0;

    ALManager *manager = ALManager::get_singleton();
    ALint channels = 0, bits = 0, frequency = 0, size = 0;
    manager->al_get_bufferi()(buffer, AL_CHANNELS, &channels);
    manager->al_get_bufferi()(buffer, AL_BITS, &bits);
    manager->al_get_bufferi()(buffer, AL_FREQUENCY, &frequency);
    manager->al_get_bufferi()(buffer, AL_SIZE, &size);

    int bytes_per_frame = channels * bits / 8;

    if (bytes_per_frame <= 0 || frequency <= 0)
        return 0;

    return (float)size / bytes_per_frame / frequency;
}

bool ALSourceHandle::is_finished() const
{
    ALManager *manager = ALManager::get_singleton();

    // OpenAL shut down (e.g. a failed device switch), which destroyed this source
    if (!manager)
        return true;

    // 0 isn't a valid AL_SOURCE_STATE, and alGetSourcei leaves it untouched if it fails (e.g. the source was invalidated by a device switch)
    ALint state = 0;
    manager->al_get_sourcei()(handle, AL_SOURCE_STATE, &state);

    if (state == 0)
    {
        VA_WARN("Lost track of a playing sound, it will be stopped. Error code: ", ALErrorToString(manager->al_get_error()()));
        return true;
    }

    // Looping sources stay AL_PLAYING, so they only finish when stopped (or when OpenAL stops them, e.g. the output device was unplugged)
    return state == AL_STOPPED;
}

void ALSourceHandle::set_gain(float gain)
{
    ALManager::get_singleton()->al_sourcef()(handle, AL_GAIN, gain);
}

void ALSourceHandle::set_pitch(float pitch)
{
    ALManager::get_singleton()->al_sourcef()(handle, AL_PITCH, pitch);
}

void ALSourceHandle::set_looping(bool value)
{
    looping = value;
    ALManager::get_singleton()->al_sourcei()(handle, AL_LOOPING, value ? 1 : 0);
}

void ALSourceHandle::set_relative(bool value)
{
    ALManager::get_singleton()->al_sourcei()(handle, AL_SOURCE_RELATIVE, value ? 1 : 0);
}

void ALSourceHandle::set_max_distance(float value)
{
    ALManager::get_singleton()->al_sourcef()(handle, AL_MAX_DISTANCE, value);
}

void ALSourceHandle::set_reference_distance(float value)
{
    ALManager::get_singleton()->al_sourcef()(handle, AL_REFERENCE_DISTANCE, value);
}

void ALSourceHandle::set_rolloff_factor(float value)
{
    ALManager::get_singleton()->al_sourcef()(handle, AL_ROLLOFF_FACTOR, value);
}

void ALSourceHandle::set_position(const Vector3 &position)
{
    ALfloat values[3] = {position.x, position.y, position.z};
    ALManager::get_singleton()->al_sourcefv()(handle, AL_POSITION, values);
}

void ALSourceHandle::set_direct_filter(ALuint filter_handle)
{
    ALManager::get_singleton()->al_sourcei()(handle, AL_DIRECT_FILTER, (ALint)filter_handle);
}

void ALSourceHandle::set_reverb_send(ALuint effect_slot_handle, ALuint reverb_filter_handle)
{
    ALManager::get_singleton()->al_source3i()(handle, AL_AUXILIARY_SEND_FILTER, (ALint)effect_slot_handle, 0, (ALint)reverb_filter_handle);
}
