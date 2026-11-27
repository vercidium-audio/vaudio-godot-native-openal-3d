// Human-created

#pragma once

#include <vaudio.h>

#include "al_functions.h"
#include "al_reverb_params.h"

class ALReverbEffect
{
private:
    ALuint effect_handle = 0;
    ALuint slot_handle = 0;

public:
    ALReverbEffect() = default;
    ~ALReverbEffect();

    // Prevent copying
    ALReverbEffect(const ALReverbEffect &) = delete;
    ALReverbEffect &operator=(const ALReverbEffect &) = delete;

    bool create();

    void destroy();

    void set_params(const VAEAXReverbParams &params);

    ALuint get_slot_handle() const
    {
        return slot_handle;
    }

    bool is_valid() const
    {
        return effect_handle != 0 && slot_handle != 0;
    }
};

// Copy the SDK's EAX params onto the OpenAL effect
VAEAXReverbParams CopyReverbParams(const VAEAXReverb *eax);
