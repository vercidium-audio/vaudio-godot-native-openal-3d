// AI-created

#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

#include "va_audio_types.h"

// Raw PCM chunks pushed on the main thread and read by a backend's audio thread, which gets silence when the queue runs dry. Same as the C# plugin's VAStreamQueue
class VAStreamQueue
{
private:
    struct Chunk
    {
        std::vector<uint8_t> data;
        size_t offset = 0;
    };

    std::mutex mutex;
    std::deque<Chunk> chunks;

    // 8-bit PCM is unsigned, so its silence is 0x80
    uint8_t silence = 0;

public:
    // Drops everything queued, and sets the format the silence is written in
    void reset(VAPcmFormat format);

    // Drops everything queued
    void clear();

    // Copies the bytes. Safe to call from any thread
    void enqueue(const uint8_t *data, int length);

    // Fills the destination with queued audio, then silence. Played chunks are dropped
    void read(uint8_t *destination, int count);
};
