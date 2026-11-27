// Human-created

#pragma once

#include "al_functions.h"

// Represents an EFX lowpass filter object
class ALFilter
{
private:
    ALuint handle = 0;
    float gain = 0.0f;
    float gain_hf = 0.0f;

public:
    ALFilter() = default;
    ~ALFilter();

    // Prevent copying
    ALFilter(const ALFilter &) = delete;
    ALFilter &operator=(const ALFilter &) = delete;

    // No-ops if OpenALBackend::has_efx() is false (rare)
    bool create(float initial_gain, float initial_gain_hf);

    void destroy();

    // gain is AL_LOWPASS_GAIN. gain_hf is relative to gain
    void set_gain(float new_gain, float new_gain_hf);

    ALuint get_handle() const
    {
        return handle;
    }

    float get_gain() const
    {
        return gain;
    }

    float get_gain_hf() const
    {
        return gain_hf;
    }

    bool is_valid() const
    {
        return handle != 0;
    }
};
