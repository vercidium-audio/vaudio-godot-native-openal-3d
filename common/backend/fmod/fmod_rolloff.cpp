// AI-created

#include "fmod_rolloff.h"

#include <algorithm>
#include <cmath>

// How far past the max distance the unclamped models keep falling, after which FMOD holds the last point's volume
static constexpr float UNCLAMPED_RANGE = 10.0f;

static constexpr float MIN_REFERENCE_DISTANCE = 0.01f;

static float curve_end(VADistanceModel model, float start, float max)
{
    float end = model == VADistanceModel::Inverse || model == VADistanceModel::Exponent ? max * UNCLAMPED_RANGE : max;
    return std::max(end, start * 1.01f);
}

// The OpenAL 1.1 distance models, clamped to 0-1 like OpenAL Soft's source gain
static float gain(VADistanceModel model, float distance, float reference, float max, float rolloff)
{
    if (model == VADistanceModel::InverseClamped || model == VADistanceModel::LinearClamped || model == VADistanceModel::ExponentClamped || model == VADistanceModel::Linear)
        distance = std::clamp(distance, reference, max);

    float result;

    switch (model)
    {
        case VADistanceModel::None:
            result = 1.0f;
            break;
        case VADistanceModel::Inverse:
        case VADistanceModel::InverseClamped:
            result = reference / (reference + rolloff * (distance - reference));
            break;
        case VADistanceModel::Linear:
        case VADistanceModel::LinearClamped:
            result = max > reference ? 1.0f - rolloff * (distance - reference) / (max - reference) : 1.0f;
            break;
        default:
            result = std::pow(distance / reference, -rolloff);
            break;
    }

    return std::isfinite(result) ? std::clamp(result, 0.0f, 1.0f) : 0.0f;
}

void FMODRolloff::fill(FMOD_VECTOR *points, VADistanceModel model, float reference_distance, float max_distance, float rolloff)
{
    float start = std::max(reference_distance, MIN_REFERENCE_DISTANCE);
    float max = std::max(max_distance, start);
    float end = curve_end(model, start, max);

    points[0] = {0.0f, 1.0f, 0.0f};

    for (int i = 1; i < POINT_COUNT; i++)
    {
        float distance = start * std::pow(end / start, (i - 1) / (float)(POINT_COUNT - 2));
        points[i] = {distance, gain(model, distance, start, max, rolloff), 0.0f};
    }
}

float FMODRolloff::curve_gain(VADistanceModel model, float distance, float reference_distance, float max_distance, float rolloff)
{
    float start = std::max(reference_distance, MIN_REFERENCE_DISTANCE);
    float max = std::max(max_distance, start);
    return gain(model, std::min(distance, curve_end(model, start, max)), start, max, rolloff);
}
