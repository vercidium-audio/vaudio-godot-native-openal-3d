// AI-created

#pragma once

#include "inc/fmod.h"
#include "inc/fmod_dsp_effects.h"
#include "inc/fmod_errors.h"

// Every FMOD Core function the backend calls. The user supplies the FMOD library, so it's loaded at runtime and these are resolved from it instead of linking
#define VA_FMOD_FUNCTIONS(X) \
    X(FMOD_System_Create) \
    X(FMOD_System_Release) \
    X(FMOD_System_SetSoftwareFormat) \
    X(FMOD_System_SetDSPBufferSize) \
    X(FMOD_System_SetDriver) \
    X(FMOD_System_GetNumDrivers) \
    X(FMOD_System_GetDriverInfo) \
    X(FMOD_System_GetRecordNumDrivers) \
    X(FMOD_System_GetRecordDriverInfo) \
    X(FMOD_System_Init) \
    X(FMOD_System_Update) \
    X(FMOD_System_GetVersion) \
    X(FMOD_System_GetMasterChannelGroup) \
    X(FMOD_System_Set3DSettings) \
    X(FMOD_System_Set3DListenerAttributes) \
    X(FMOD_System_CreateSound) \
    X(FMOD_System_CreateDSPByType) \
    X(FMOD_System_PlaySound) \
    X(FMOD_Sound_Release) \
    X(FMOD_Sound_GetUserData) \
    X(FMOD_ChannelGroup_SetVolume) \
    X(FMOD_Channel_Stop) \
    X(FMOD_Channel_SetPaused) \
    X(FMOD_Channel_GetPaused) \
    X(FMOD_Channel_IsPlaying) \
    X(FMOD_Channel_SetVolume) \
    X(FMOD_Channel_SetPitch) \
    X(FMOD_Channel_SetMode) \
    X(FMOD_Channel_SetLoopCount) \
    X(FMOD_Channel_GetPosition) \
    X(FMOD_Channel_SetPosition) \
    X(FMOD_Channel_Set3DAttributes) \
    X(FMOD_Channel_Set3DCustomRolloff) \
    X(FMOD_Channel_AddDSP) \
    X(FMOD_Channel_RemoveDSP) \
    X(FMOD_DSP_Release) \
    X(FMOD_DSP_SetParameterFloat)

// The resolved functions, called as va_fmod.FMOD_System_Create(...). Only valid once FMODLibrary::load has succeeded
struct FMODApi
{
#define VA_FMOD_DECLARE(name) decltype(&::name) name = nullptr;
    VA_FMOD_FUNCTIONS(VA_FMOD_DECLARE)
#undef VA_FMOD_DECLARE
};

inline FMODApi va_fmod;
