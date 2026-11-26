// AI-created

#pragma once

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/ref.hpp>

#include <cstdint>
#include <vector>

// Decodes any AudioStream into interleaved stereo 16-bit PCM at the AudioServer's mix rate (Godot's mixer always outputs stereo), ready for VAAudioBackendBase::create_sound. Safe to call from a worker thread. Logs and returns false if it decoded nothing
bool va_decode_audio_stream(const godot::Ref<godot::AudioStream> &stream, std::vector<int16_t> &out_pcm16, int &out_sample_rate);
