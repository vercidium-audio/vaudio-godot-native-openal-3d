// AI-created

#include "al_filter.h"

#include "va_engine_util.h"
#include "openal_backend.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>

using namespace godot;

ALFilter::~ALFilter()
{
    destroy();
}

bool ALFilter::create(float initial_gain, float initial_gain_hf)
{
    OpenALBackend *manager = OpenALBackend::get_singleton();

    if (!manager || !manager->is_initialised())
    {
        VA_BACKEND_ERROR("ALFilter::create called before the OpenAL device is initialized");
        return false;
    }

    if (!manager->has_efx())
    {
        // Not an error - devices without ALC_EXT_EFX just play without muffling.
        return false;
    }

    ALuint new_handle = 0;
    manager->al_gen_filters()(1, &new_handle);

    if (new_handle == 0)
    {
        VA_BACKEND_ERROR("alGenFilters failed, AL error ", (int)manager->al_get_error()());
        return false;
    }

    handle = new_handle;
    manager->al_filteri()(handle, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
    set_gain(initial_gain, initial_gain_hf);
    return true;
}

void ALFilter::destroy()
{
    OpenALBackend *manager = OpenALBackend::get_singleton();

    if (handle != 0 && manager && manager->has_efx())
        manager->delete_filter(handle);

    handle = 0;
}

void ALFilter::set_gain(float new_gain, float new_gain_hf)
{
    gain = new_gain;
    gain_hf = std::min(1.0f, new_gain_hf / std::max(0.01f, new_gain));

    if (!is_valid())
    {
        return;
    }

    OpenALBackend *manager = OpenALBackend::get_singleton();
    manager->al_filterf()(handle, AL_LOWPASS_GAIN, gain);
    manager->al_filterf()(handle, AL_LOWPASS_GAINHF, gain_hf);
}
