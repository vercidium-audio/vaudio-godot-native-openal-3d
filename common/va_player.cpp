// AI-created

#include "va_player.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "backend/va_audio_backend.h"
#include "backend/va_audio_decode.h"
#include "va_engine_util.h"

#include <algorithm>
#include <cmath>

void VAPlayer::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_streams"), &VAPlayer::get_streams);
    ClassDB::bind_method(D_METHOD("set_streams", "value"), &VAPlayer::set_streams);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "streams", PROPERTY_HINT_TYPE_STRING, String::num(Variant::OBJECT) + "/" + String::num(PROPERTY_HINT_RESOURCE_TYPE) + ":AudioStream"), "set_streams", "get_streams");

    // Script-only alias for `streams` - not exposed in the inspector, see va_player.h's get_stream()/set_stream().
    ClassDB::bind_method(D_METHOD("get_stream"), &VAPlayer::get_stream);
    ClassDB::bind_method(D_METHOD("set_stream", "value"), &VAPlayer::set_stream);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stream", PROPERTY_HINT_RESOURCE_TYPE, "AudioStream", PROPERTY_USAGE_NONE), "set_stream", "get_stream");

    ClassDB::bind_method(D_METHOD("get_gain"), &VAPlayer::get_gain);
    ClassDB::bind_method(D_METHOD("set_gain", "value"), &VAPlayer::set_gain);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gain", PROPERTY_HINT_RANGE, "0.0,1.0,0.01,or_greater"), "set_gain", "get_gain");

    ClassDB::bind_method(D_METHOD("get_pitch"), &VAPlayer::get_pitch);
    ClassDB::bind_method(D_METHOD("set_pitch", "value"), &VAPlayer::set_pitch);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch", PROPERTY_HINT_RANGE, "0.01,4.0,0.01"), "set_pitch", "get_pitch");

    // Script-only alias for `pitch` - not exposed in the inspector, see va_player.h's get_pitch_scale()/set_pitch_scale().
    ClassDB::bind_method(D_METHOD("get_pitch_scale"), &VAPlayer::get_pitch_scale);
    ClassDB::bind_method(D_METHOD("set_pitch_scale", "value"), &VAPlayer::set_pitch_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch_scale", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_pitch_scale", "get_pitch_scale");

    // Script-only alias for `gain` - not exposed in the inspector, see va_player.h's get_volume_db().
    ClassDB::bind_method(D_METHOD("get_volume_db"), &VAPlayer::get_volume_db);
    ClassDB::bind_method(D_METHOD("set_volume_db", "value"), &VAPlayer::set_volume_db);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume_db", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_volume_db", "get_volume_db");

    ClassDB::bind_method(D_METHOD("get_pitch_randomness"), &VAPlayer::get_pitch_randomness);
    ClassDB::bind_method(D_METHOD("set_pitch_randomness", "value"), &VAPlayer::set_pitch_randomness);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch_randomness", PROPERTY_HINT_RANGE, "1.0,4.0,0.01"), "set_pitch_randomness", "get_pitch_randomness");

    ClassDB::bind_method(D_METHOD("get_volume_randomness_db"), &VAPlayer::get_volume_randomness_db);
    ClassDB::bind_method(D_METHOD("set_volume_randomness_db", "value"), &VAPlayer::set_volume_randomness_db);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume_randomness_db", PROPERTY_HINT_RANGE, "0.0,24.0,0.1"), "set_volume_randomness_db", "get_volume_randomness_db");

    ClassDB::bind_method(D_METHOD("get_playback_no_repeat"), &VAPlayer::get_playback_no_repeat);
    ClassDB::bind_method(D_METHOD("set_playback_no_repeat", "value"), &VAPlayer::set_playback_no_repeat);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "playback_no_repeat"), "set_playback_no_repeat", "get_playback_no_repeat");

    ClassDB::bind_method(D_METHOD("get_reverb_only"), &VAPlayer::get_reverb_only);
    ClassDB::bind_method(D_METHOD("set_reverb_only", "value"), &VAPlayer::set_reverb_only);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reverb_only"), "set_reverb_only", "get_reverb_only");

    ClassDB::bind_method(D_METHOD("get_looping"), &VAPlayer::get_looping);
    ClassDB::bind_method(D_METHOD("set_looping", "value"), &VAPlayer::set_looping);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "looping"), "set_looping", "get_looping");

    ClassDB::bind_method(D_METHOD("get_autoplay"), &VAPlayer::get_autoplay);
    ClassDB::bind_method(D_METHOD("set_autoplay", "value"), &VAPlayer::set_autoplay);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "get_autoplay");

    ClassDB::bind_method(D_METHOD("get_stream_paused"), &VAPlayer::get_stream_paused);
    ClassDB::bind_method(D_METHOD("set_stream_paused", "value"), &VAPlayer::set_stream_paused);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "stream_paused"), "set_stream_paused", "get_stream_paused");

    ClassDB::bind_method(D_METHOD("play", "start_time"), &VAPlayer::play, DEFVAL(0.0));
    ClassDB::bind_method(D_METHOD("stop"), &VAPlayer::stop);
    ClassDB::bind_method(D_METHOD("is_playing"), &VAPlayer::is_playing);
    ClassDB::bind_method(D_METHOD("seek", "time"), &VAPlayer::seek);
    ClassDB::bind_method(D_METHOD("get_playback_time"), &VAPlayer::get_playback_time);

    // Script-only alias for get_playback_time() matching AudioStreamPlayer3D's get_playback_position()
    ClassDB::bind_method(D_METHOD("get_playback_position"), &VAPlayer::get_playback_time);
    ClassDB::bind_method(D_METHOD("has_reverb"), &VAPlayer::has_reverb);
    ClassDB::bind_method(D_METHOD("is_dry_output_enabled"), &VAPlayer::is_dry_output_enabled);
}

VAPlayer::VAPlayer()
{
}

VAPlayer::~VAPlayer()
{
    // A decode task holds a raw pointer to this node via userdata - must not let it run after this node is destroyed.
    if (decode_task_id != WorkerThreadPool::INVALID_TASK_ID)
        WorkerThreadPool::get_singleton()->wait_for_task_completion(decode_task_id);

    // Voices go before the sounds they play
    voices.clear();
}

void VAPlayer::_ready()
{
    if (autoplay && !IS_EDITOR_HINT())
    {
        play();
    }
}

void VAPlayer::_process(double delta)
{
    poll_decode_task();

    // Voices never remove themselves once finished - without this, every play() call permanently leaks one.
    voices.erase(
        std::remove_if(voices.begin(), voices.end(), [](const VAPlayerVoice &voice)
                        { return voice.get()->is_finished(); }),
        voices.end());

    // Picks up this source's or the VAWorld's reverb_only changing, including on sources that don't update their filter every frame
    if (update_dry_output())
    {
        for (auto &voice : voices)
            apply_filters(*voice.get());
    }
}

bool VAPlayer::update_dry_output()
{
    VAAudioBackend *manager = VAAudioBackend::get_singleton();
    bool enabled = !reverb_only && !(manager && manager->get_reverb_only());
    bool changed = enabled != dry_output_enabled;
    dry_output_enabled = enabled;
    return changed;
}

void VAPlayer::apply_filters(VAVoice &voice) const
{
    VALowPass filter = muffling.value_or(VALowPass::full());

    // For reverb in other rooms, the clean/unfiltered sound is sent to the reverb effect, and then the reverb effect's output gain is reduced based on vaEAXReverbGetRelativeGain
    VALowPass send = full_reverb ? VALowPass::full() : filter;
    VALowPass direct = dry_output_enabled ? filter : VALowPass::silence();

    voice.set_filters(direct, reverb_effect, send);
}

void VAPlayer::configure_listener_relative_voice(VAVoice &voice)
{
    voice.set_relative(true);
    voice.set_position(Vector3());
}

void VAPlayer::clear_voices()
{
    voices.clear();
}

void VAPlayer::decode_stream_task(void *userdata)
{
    VAPlayer *node = static_cast<VAPlayer *>(userdata);
    node->decode_succeeded = va_decode_audio_stream(node->decoding_stream, node->decoding_pcm, node->decoding_sample_rate);
}

bool VAPlayer::ensure_stream_decode_started()
{
    if (streams.is_empty())
    {
        // Voices still playing a sound keep it alive
        decoded_sounds.clear();
        decoded_streams.clear();
        pending_streams.clear();
        return false;
    }

    // Drop any cached decode whose stream is no longer in `streams` (e.g. removed or reassigned).
    for (size_t i = 0; i < decoded_streams.size();)
    {
        bool still_wanted = false;

        for (int j = 0; j < streams.size(); j++)
        {
            if (Ref<AudioStream>(streams[j]) == decoded_streams[i])
            {
                still_wanted = true;
                break;
            }
        }

        if (still_wanted)
        {
            i++;
        }
        else
        {
            decoded_streams.erase(decoded_streams.begin() + i);
            decoded_sounds.erase(decoded_sounds.begin() + i);
        }
    }

    // Queue a decode for every stream that isn't already decoded, in flight, or already queued.
    for (int i = 0; i < streams.size(); i++)
    {
        Ref<AudioStream> wanted = streams[i];

        if (wanted.is_null())
            continue;

        bool already_decoded = false;

        for (const Ref<AudioStream> &decoded : decoded_streams)
        {
            if (decoded == wanted)
            {
                already_decoded = true;
                break;
            }
        }

        if (already_decoded || wanted == decoding_stream)
            continue;

        bool already_queued = false;

        for (const Ref<AudioStream> &pending : pending_streams)
        {
            if (pending == wanted)
            {
                already_queued = true;
                break;
            }
        }

        if (!already_queued)
            pending_streams.push_back(wanted);
    }

    if (pending_streams.empty() && decode_task_id == WorkerThreadPool::INVALID_TASK_ID)
        return true;

    // A decode is already in flight - let it finish rather than starting a second, redundant one.
    if (decode_task_id != WorkerThreadPool::INVALID_TASK_ID)
        return true;

    decoding_stream = pending_streams.front();
    pending_streams.erase(pending_streams.begin());
    decode_task_id = WorkerThreadPool::get_singleton()->add_native_task(
        &VAPlayer::decode_stream_task, this, false, "vaudio stream decode");

    return true;
}

void VAPlayer::poll_decode_task()
{
    if (decode_task_id == WorkerThreadPool::INVALID_TASK_ID)
        return;

    if (!WorkerThreadPool::get_singleton()->is_task_completed(decode_task_id))
        return;

    WorkerThreadPool::get_singleton()->wait_for_task_completion(decode_task_id);
    decode_task_id = WorkerThreadPool::INVALID_TASK_ID;

    std::shared_ptr<VASound> sound;

    if (decode_succeeded)
    {
        // Always interleaved stereo, since Godot's mixer upmixes mono
        VAAudioBackend *manager = VAAudioBackend::get_singleton();
        sound = manager ? manager->create_sound(decoding_pcm.data(), (int64_t)decoding_pcm.size(), 2, decoding_sample_rate) : nullptr;
    }

    // The backend has its own copy now
    decoding_pcm.clear();
    decoding_pcm.shrink_to_fit();

    if (sound)
    {
        decoded_streams.push_back(decoding_stream);
        decoded_sounds.push_back(std::move(sound));
    }
    else
    {
        String stream_path = decoding_stream.is_valid() ? decoding_stream->get_path() : String();

        if (stream_path.is_empty())
            stream_path = "<no resource path, likely a stream created at runtime>";

        if (decode_succeeded)
            VA_ERROR_NAMED("Failed to upload the decoded stream to the audio backend: ", stream_path);
        else
            VA_ERROR_NAMED("Failed to decode the stream: ", stream_path);
    }

    decoding_stream.unref();

    // More streams still queued - start the next one rather than waiting for another play()/poll cycle.
    if (!pending_streams.empty())
    {
        decoding_stream = pending_streams.front();
        pending_streams.erase(pending_streams.begin());
        decode_task_id = WorkerThreadPool::get_singleton()->add_native_task(
            &VAPlayer::decode_stream_task, this, false, "vaudio stream decode");
        return;
    }

    if (play_requested)
    {
        play_requested = false;
        start_playing(requested_start_time);
    }
}

int VAPlayer::pick_stream_index() const
{
    if (decoded_sounds.empty())
        return -1;

    if (decoded_sounds.size() == 1)
        return 0;

    int index = (int)UtilityFunctions::randi_range(0, (int64_t)decoded_sounds.size() - 1);

    if (playback_no_repeat && index == last_played_index)
        index = (index + 1) % (int)decoded_sounds.size();

    return index;
}

bool VAPlayer::start_playing(float start_time)
{
    int index = pick_stream_index();

    // No decoded sound to play (streams failed to decode/upload, or nothing was ever assigned).
    if (index < 0)
        return false;

    last_played_index = index;

    VAPlayerVoice entry;
    entry.sound = decoded_sounds[index];

    // The backend already logged the reason for failure (e.g. too many sounds playing at once)
    entry.owned_voice = entry.sound->create_voice(true);

    if (!entry.owned_voice)
        return false;

    VAVoice &voice = *entry.owned_voice;

    // Matches AudioStreamRandomizer's random_pitch/random_volume_offset_db; a randomness of 1.0/0.0 collapses randf_range to the single input value.
    float randomized_pitch = pitch * (float)UtilityFunctions::randf_range(1.0 / pitch_randomness, pitch_randomness);
    float randomized_gain = gain * (float)UtilityFunctions::db_to_linear(UtilityFunctions::randf_range(-volume_randomness_db, volume_randomness_db));

    voice.set_gain(randomized_gain);
    voice.set_pitch(randomized_pitch);
    voice.set_looping(looping);

    configure_voice(voice);

    update_dry_output();
    apply_filters(voice);

    // An initial voice starts from its offset once played
    if (start_time > 0)
        seek_voice(voice, start_time);

    if (!stream_paused)
        voice.play();

    voices.push_back(std::move(entry));
    return true;
}

bool VAPlayer::play(float start_time)
{
    bool need_decode = !ensure_stream_decode_started();

    if (need_decode)
        return false;

    if (decode_task_id == WorkerThreadPool::INVALID_TASK_ID && pending_streams.empty())
        return start_playing(start_time);

    // A decode is in flight - poll_decode_task() will start playback once every entry in `streams` has finished decoding.
    play_requested = true;
    requested_start_time = start_time;
    return true;
}

void VAPlayer::set_stream_paused(bool value)
{
    if (stream_paused == value)
        return;

    stream_paused = value;

    for (auto &voice : voices)
        apply_paused(*voice.get());
}

void VAPlayer::apply_paused(VAVoice &voice) const
{
    VAVoiceState state = voice.get_state();

    if (stream_paused)
    {
        if (state == VAVoiceState::Playing)
            voice.pause();
    }
    // Not a stopped one, which would restart from the beginning
    else if (state == VAVoiceState::Paused || state == VAVoiceState::Initial)
        voice.play();
}

void VAPlayer::seek(float time)
{
    for (auto &voice : voices)
        seek_voice(*voice.get(), time);
}

void VAPlayer::seek_voice(VAVoice &voice, float time)
{
    float length = voice.get_length();

    if (length <= 0)
        return;

    time = std::max(0.0f, time);

    if (time >= length)
    {
        if (!voice.get_looping())
        {
            voice.stop();
            return;
        }

        time = std::fmod(time, length);
    }

    voice.set_playback_position(time);
}

float VAPlayer::get_playback_time() const
{
    if (voices.empty())
        return 0;

    return voices.back().get()->get_playback_position();
}

void VAPlayer::stop()
{
    play_requested = false;

    for (auto &voice : voices)
        voice.get()->stop();

    // Release them now rather than waiting for _process to see them finish
    clear_voices();
}

bool VAPlayer::is_playing() const
{
    return !stream_paused && has_sound();
}

bool VAPlayer::has_sound() const
{
    for (auto &voice : voices)
        if (!voice.get()->is_finished())
            return true;

    return false;
}

void VAPlayer::update_filter(float new_gain, float new_gain_hf, bool fullReverb)
{
    muffling = VALowPass{new_gain, new_gain_hf};
    full_reverb = fullReverb;
    update_dry_output();

    for (auto &voice : voices)
        apply_filters(*voice.get());
}

void VAPlayer::set_gain(float value)
{
    gain = value;

    for (auto &voice : voices)
        voice.get()->set_gain(value);
}

void VAPlayer::set_pitch(float value)
{
    pitch = value;

    for (auto &voice : voices)
        voice.get()->set_pitch(value);
}

void VAPlayer::set_looping(bool value)
{
    looping = value;

    for (auto &voice : voices)
        voice.get()->set_looping(value);
}

void VAPlayer::set_autoplay(bool value)
{
    autoplay = value;
}

float VAPlayer::get_volume_db() const
{
    return (float)UtilityFunctions::linear_to_db(gain);
}

void VAPlayer::set_volume_db(float value)
{
    set_gain((float)UtilityFunctions::db_to_linear(value));
}
