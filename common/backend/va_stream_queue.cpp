// AI-created

#include "va_stream_queue.h"

#include <algorithm>
#include <cstring>

void VAStreamQueue::reset(VAPcmFormat format)
{
    std::lock_guard<std::mutex> lock(mutex);
    chunks.clear();
    silence = va_pcm_bytes_per_sample(format) == 1 ? 0x80 : 0;
}

void VAStreamQueue::enqueue(const uint8_t *data, int length)
{
    if (length <= 0)
        return;

    Chunk chunk;
    chunk.data.assign(data, data + length);

    std::lock_guard<std::mutex> lock(mutex);
    chunks.push_back(std::move(chunk));
}

void VAStreamQueue::clear()
{
    std::lock_guard<std::mutex> lock(mutex);
    chunks.clear();
}

void VAStreamQueue::read(uint8_t *destination, int count)
{
    std::lock_guard<std::mutex> lock(mutex);

    while (count > 0 && !chunks.empty())
    {
        Chunk &chunk = chunks.front();
        size_t bytes = std::min((size_t)count, chunk.data.size() - chunk.offset);

        std::memcpy(destination, chunk.data.data() + chunk.offset, bytes);
        chunk.offset += bytes;
        destination += bytes;
        count -= (int)bytes;

        if (chunk.offset == chunk.data.size())
            chunks.pop_front();
    }

    if (count > 0)
        std::memset(destination, silence, (size_t)count);
}
