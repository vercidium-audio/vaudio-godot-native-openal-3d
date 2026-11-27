// Human-reviewed

#pragma once

#include <vaudio.h>

#include "al_functions.h"

// TODO - move this to a separate file
struct VAEAXReverbParams
{
    float density = 1.0f;
    float diffusion = 1.0f;
    float gain = 1.0f;
    float gainHF = 1.0f;
    float gainLF = 1.0f;
    float decayTime = 1.0f;
    float decayHFRatio = 1.0f;
    float decayLFRatio = 1.0f;
    float reflectionsGain = 1.0f;
    float reflectionsDelay = 0.0f;
    float reflectionsPan[3] = {0.0f, 0.0f, 0.0f};
    float lateReverbGain = 1.0f;
    float lateReverbDelay = 0.0f;
    float lateReverbPan[3] = {0.0f, 0.0f, 0.0f};
    float echoTime = 0.25f;
    float echoDepth = 0.0f;
    float modulationTime = 0.25f;
    float modulationDepth = 0.0f;
    float airAbsorptionGainHF = 0.994f;
    float hfReference = 5000.0f;
    float lfReference = 250.0f;
    float roomRolloffFactor = 0.0f;
    int decayHFLimit = 1; // AL_TRUE

    // Used to muffle a grouped EAX zone. Stays 1.0 for the VAListener
    float effectSlotGain = 1.0f;
};

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

// Copiy the SDK's EAX params onto the OpenAL effect
VAEAXReverbParams CopyReverbParams(const VAEAXReverb *eax);
