// AI-created

#include "al_stream_buffer.h"

#include "va_engine_util.h"
#include "openal_backend.h"

ALStreamBuffer::~ALStreamBuffer()
{
    destroy();
}

void ALStreamBuffer::destroy()
{
    OpenALBackend *manager = OpenALBackend::get_singleton();

    if (handle != 0 && manager)
        manager->delete_buffer(handle);

    handle = 0;
    queue.clear();
}

bool ALStreamBuffer::create(VAPcmFormat format, int frequency)
{
    OpenALBackend *manager = OpenALBackend::get_initialized_singleton("open the stream");

    if (!manager)
        return false;

    // Only missing if the bundled OpenAL Soft (1.21+) was swapped for another OpenAL library, which the startup warning already reported. Logged once since every stream would fail the same way
    if (!manager->al_buffer_callback_soft())
    {
        static bool logged = false;

        if (!logged)
        {
            logged = true;
            VA_BACKEND_ERROR("Can't open streams because the OpenAL library doesn't support AL_SOFT_callback_buffer. Use the OpenAL Soft library that ships in the plugin's bin folder");
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
    queue.reset(format);

    ALuint new_handle = manager->gen_buffer();

    if (new_handle == 0)
        return false;

    // Clear any error left over from an earlier AL call so it isn't misattributed to alBufferCallbackSOFT below
    manager->al_get_error()();

    manager->al_buffer_callback_soft()(new_handle, OpenALBackend::to_al(format), frequency, &ALStreamBuffer::buffer_callback, this);

    ALenum error = manager->al_get_error()();

    if (error != AL_NO_ERROR)
    {
        VA_BACKEND_ERROR("Failed to open the stream. Error code: ", ALErrorToString(error));
        manager->delete_buffer(new_handle);
        return false;
    }

    handle = new_handle;
    return true;
}

ALsizei AL_APIENTRY ALStreamBuffer::buffer_callback(ALvoid *userptr, ALvoid *sampledata, ALsizei numbytes) noexcept
{
    static_cast<ALStreamBuffer *>(userptr)->queue.read(static_cast<uint8_t *>(sampledata), numbytes);
    return numbytes;
}
