// AI-created

#include "va_audio_decode.h"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/audio_stream_playback.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <algorithm>

#include "va_engine_util.h"

using namespace godot;

// Pull frames from playback in fixed-size chunks rather than one huge mix_audio() call, to bound the temporary allocation.
static constexpr int32_t MIX_CHUNK_FRAMES = 8192;

static int16_t float_to_pcm16(float sample)
{
    float clamped = std::clamp(sample, -1.0f, 1.0f);
    return (int16_t)(clamped * 32767.0f);
}

bool va_decode_audio_stream(const Ref<AudioStream> &stream, std::vector<int16_t> &out_pcm16, int &out_sample_rate)
{
    out_pcm16.clear();

    if (stream.is_null())
    {
        VA_ERROR("Can't decode a null AudioStream");
        return false;
    }

    Ref<AudioStreamPlayback> playback = stream->instantiate_playback();

    if (playback.is_null())
    {
        VA_ERROR("AudioStream failed to instantiate a playback (unsupported/corrupt file?)");
        return false;
    }

    float mix_rate = AudioServer::get_singleton()->get_mix_rate();
    double length_seconds = stream->get_length();

    // Some streams report a zero/negative length; fall back to a generous frame cap so a broken stream can't loop forever.
    int64_t expected_frames = length_seconds > 0.0
        ? (int64_t)(length_seconds * mix_rate) + MIX_CHUNK_FRAMES
        : (int64_t)mix_rate * 60 * 10; // 10 minute safety cap

    playback->start();

    out_pcm16.reserve((size_t)std::min<int64_t>(expected_frames, (int64_t)mix_rate * 60) * 2);

    int64_t frames_pulled = 0;

    while (frames_pulled < expected_frames)
    {
        PackedVector2Array chunk = playback->mix_audio(1.0f, MIX_CHUNK_FRAMES);

        if (chunk.size() == 0)
        {
            break;
        }

        for (int64_t i = 0; i < chunk.size(); i++)
        {
            Vector2 frame = chunk[i];
            out_pcm16.push_back(float_to_pcm16(frame.x));
            out_pcm16.push_back(float_to_pcm16(frame.y));
        }

        frames_pulled += chunk.size();

        // Fewer frames than requested means the stream has ended (Godot's documented pull-based mix contract).
        if (chunk.size() < MIX_CHUNK_FRAMES)
        {
            break;
        }
    }

    playback->stop();

    if (out_pcm16.empty())
    {
        VA_ERROR("AudioStream decoded to zero frames");
        return false;
    }

    out_sample_rate = (int)mix_rate;
    return true;
}
