// AI-created

#pragma once

#include <algorithm>

// How a sound's volume falls off with distance, between its reference and max distance. Each audio backend recreates these models. The values are what VAWorld's distance_model stores
enum class VADistanceModel
{
    None,
    Inverse,
    InverseClamped,
    Linear,
    LinearClamped,
    Exponent,
    ExponentClamped,
};

// PROPERTY_HINT_ENUM string matching VADistanceModel's values
#define VA_DISTANCE_MODEL_HINT "None,Inverse,InverseClamped,Linear,LinearClamped,Exponent,ExponentClamped"

// The channel count and sample type of raw PCM audio, e.g. pushed into a VAStreamSource or captured by a VAInputStreamSource. 8-bit samples are unsigned
enum class VAPcmFormat
{
    Mono8,
    Mono16,
    Stereo8,
    Stereo16,
    MonoFloat,
    StereoFloat,
};

inline int va_pcm_channels(VAPcmFormat format)
{
    return format == VAPcmFormat::Mono8 || format == VAPcmFormat::Mono16 || format == VAPcmFormat::MonoFloat ? 1 : 2;
}

inline int va_pcm_bytes_per_sample(VAPcmFormat format)
{
    switch (format)
    {
        case VAPcmFormat::Mono8:
        case VAPcmFormat::Stereo8:
            return 1;
        case VAPcmFormat::Mono16:
        case VAPcmFormat::Stereo16:
            return 2;
        default:
            return 4;
    }
}

inline int va_pcm_bytes_per_frame(VAPcmFormat format)
{
    return va_pcm_channels(format) * va_pcm_bytes_per_sample(format);
}

// Low and high frequency gains (0-1) applied to a voice's direct path or reverb send, from the SDK's muffling results. Not called VALowPassFilter like the C# plugin's, since the C SDK already has a struct with that name
struct VALowPass
{
    float gain_lf = 1.0f;
    float gain_hf = 1.0f;

    static VALowPass full()
    {
        return {1.0f, 1.0f};
    }

    static VALowPass silence()
    {
        return {0.0f, 0.0f};
    }

    // gain_hf relative to gain_lf, clamped to 1, as the nodes' gain_hf properties report it
    float relative_gain_hf() const
    {
        return std::min(1.0f, gain_hf / std::max(0.01f, gain_lf));
    }

    bool operator==(const VALowPass &other) const
    {
        return gain_lf == other.gain_lf && gain_hf == other.gain_hf;
    }

    bool operator!=(const VALowPass &other) const
    {
        return !(*this == other);
    }
};

enum class VAVoiceState
{
    Initial,
    Playing,
    Paused,
    Stopped,
};
