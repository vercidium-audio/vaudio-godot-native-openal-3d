// AI-created

#pragma once

#include "al_functions.h"
#include "backend/va_stream_queue.h"

// An OpenAL buffer whose samples come from a VAStreamQueue, via alBufferCallbackSOFT
class ALStreamBuffer
{
private:
    ALuint handle = 0;

    VAStreamQueue queue;

    // Free function OpenAL invokes (ALBUFFERCALLBACKTYPESOFT signature) on its mixer thread; userptr is always `this`
    static ALsizei AL_APIENTRY buffer_callback(ALvoid *userptr, ALvoid *sampledata, ALsizei numbytes) noexcept;

public:
    ALStreamBuffer() = default;
    ~ALStreamBuffer();

    ALStreamBuffer(const ALStreamBuffer &) = delete;
    ALStreamBuffer &operator=(const ALStreamBuffer &) = delete;

    // Allocates the OpenAL buffer and registers this instance's callback via alBufferCallbackSOFT. Returns false (with a Godot error already logged) if AL_SOFT_callback_buffer isn't available or buffer creation fails. Destroys any existing buffer first.
    bool create(VAPcmFormat format, int frequency);

    // Deletes the OpenAL buffer (if any) and drops every queued chunk. Safe to call more than once, even if create() was never called or failed.
    void destroy();

    // Copies length bytes starting at data into the queue. Safe to call from any thread; the caller retains ownership of data.
    void enqueue(const uint8_t *data, int length)
    {
        queue.enqueue(data, length);
    }

    ALuint get_handle() const
    {
        return handle;
    }

    bool is_valid() const
    {
        return handle != 0;
    }
};
