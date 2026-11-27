// AI-created

#pragma once

#include "backend/va_audio_types.h"
#include "fmod_api.h"

// FMOD has no per-channel rolloff factor or OpenAL-style distance models, so each voice gets a custom rolloff curve sampled from the OpenAL formulas. Same as the C# plugin's FMODRolloff
namespace FMODRolloff
{
constexpr int POINT_COUNT = 32;

// Volume is 1 up to the reference distance in every model, then the points spread out geometrically, since the inverse and exponent curves change fastest near the start
void fill(FMOD_VECTOR *points, VADistanceModel model, float reference_distance, float max_distance, float rolloff);

// The volume the voice's rolloff curve gives at this distance (ignoring FMOD's interpolation between its points), for things FMOD's rolloff doesn't reach, like a pre-fader reverb send
float curve_gain(VADistanceModel model, float distance, float reference_distance, float max_distance, float rolloff);
}
