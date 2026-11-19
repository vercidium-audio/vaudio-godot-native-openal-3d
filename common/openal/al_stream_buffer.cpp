// Human-reviewed

#include "al_stream_buffer.h"

#include "../va_engine_util.h"
#include "al_manager.h"

#include <algorithm>
#include <cstring>

ALStreamBuffer::~ALStreamBuffer()
{
    destroy();
}

void ALStreamBuffer::destroy()
{
    ALManager *manager = ALManager::get_singleton();

    if (handle != 0 && manager)
        manager->delete_buffer(handle);

    handle = 0;

    clear();

    std::lock_guard<std::mutex> lock(chunks_mutex);
    used_chunks.clear();
}

void ALStreamBuffer::clear()
{
    std::lock_guard<std::mutex> lock(chunks_mutex);
    pending_chunks.clear();
    has_partial_chunk = false;
}

bool ALStreamBuffer::create(ALenum format, int frequency)
{
    ALManager *manager = ALManager::get_initialized_singleton("open the stream");

    if (!manager)
        return false;

    // Only missing if the bundled OpenAL Soft (1.21+) was swapped for another OpenAL library, which the startup warning already reported. Logged once since every stream would fail the same way
    if (!manager->al_buffer_callback_soft())
    {
        static bool logged = false;

        if (!logged)
        {
            logged = true;
            VA_ERROR("Can't open streams because the OpenAL library doesn't support AL_SOFT_callback_buffer. Use the OpenAL Soft library that ships in the plugin's bin folder");
        }

        return false;
    }

    // alBufferCallbackSOFT also rejects this with AL_INVALID_VALUE, but checking first gives a clearer error
    if (frequency <= 0)
    {
        VA_ERROR("Can't open the stream because its frequency is ", frequency, " Hz. It must be the sample rate of the pushed audio, e.g. 44100");
        return false;
    }

    destroy();

    ALuint new_handle = manager->gen_buffer();

    if (new_handle == 0)
        return false;

    // Clear any error left over from an earlier AL call so it isn't misattributed to alBufferCallbackSOFT below
    manager->al_get_error()();

    manager->al_buffer_callback_soft()(new_handle, format, frequency, &ALStreamBuffer::buffer_callback, this);

    ALenum error = manager->al_get_error()();

    if (error != AL_NO_ERROR)
    {
        // The buffer is new and the callback isn't null, so AL_INVALID_ENUM (unknown or non-PCM format) is the only error the caller can cause
        if (error == AL_INVALID_ENUM)
            VA_ERROR("Can't open the stream because format ", (int)format, " isn't supported. Use 4353 (AL_FORMAT_MONO16), 4355 (AL_FORMAT_STEREO16), 4352 (AL_FORMAT_MONO8) or 4354 (AL_FORMAT_STEREO8)");
        else
            VA_ERROR("Failed to open the stream. Error code: ", ALErrorToString(error));

        manager->delete_buffer(new_handle);
        return false;
    }

    handle = new_handle;
    return true;
}

void ALStreamBuffer::enqueue(const uint8_t *data, int length)
{
    if (length <= 0)
        return;

    Chunk chunk;
    chunk.data.assign(data, data + length);
    chunk.offset = 0;
    chunk.length = length;

    // Called on the main thread, while on_buffer_callback pops pending_chunks on OpenAL Soft's mixer thread
    std::lock_guard<std::mutex> lock(chunks_mutex);
    pending_chunks.push_back(std::move(chunk));
}

bool ALStreamBuffer::try_get_used_chunk(std::vector<uint8_t> &out_data)
{
    std::lock_guard<std::mutex> lock(chunks_mutex);

    if (used_chunks.empty())
        return false;

    out_data = std::move(used_chunks.front().data);
    used_chunks.pop_front();
    return true;
}

ALsizei AL_APIENTRY ALStreamBuffer::buffer_callback(ALvoid *userptr, ALvoid *sampledata, ALsizei numbytes) noexcept
{
    return static_cast<ALStreamBuffer *>(userptr)->on_buffer_callback(sampledata, numbytes);
}

ALsizei ALStreamBuffer::on_buffer_callback(ALvoid *sampledata, ALsizei numbytes)
{
    uint8_t *write = static_cast<uint8_t *>(sampledata);
    ALsizei bytes_left = numbytes;

    while (bytes_left > 0)
    {
        Chunk chunk;

        if (has_partial_chunk)
        {
            chunk = std::move(partial_chunk);
            has_partial_chunk = false;
        }
        else
        {
            std::lock_guard<std::mutex> lock(chunks_mutex);

            if (pending_chunks.empty())
                break;

            chunk = std::move(pending_chunks.front());
            pending_chunks.pop_front();
        }

        int bytes_to_write = std::min(chunk.length, (int)bytes_left);

        std::memcpy(write, chunk.data.data() + chunk.offset, (size_t)bytes_to_write);

        write += bytes_to_write;
        bytes_left -= bytes_to_write;

        if (bytes_to_write < chunk.length)
        {
            // Only partially consumed - keep the remainder for the next callback call instead of moving it to used_chunks yet.
            chunk.offset += bytes_to_write;
            chunk.length -= bytes_to_write;
            partial_chunk = std::move(chunk);
            has_partial_chunk = true;
        }
        else
        {
            std::lock_guard<std::mutex> lock(chunks_mutex);
            used_chunks.push_back(std::move(chunk));
        }
    }

    // Not enough pending data to fill the request - pad the rest with silence rather than starving the source.
    if (bytes_left > 0)
    {
        std::memset(write, 0, (size_t)bytes_left);
    }

    return numbytes;
}
