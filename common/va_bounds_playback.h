#pragma once

class ALSource;

// Autoplay and world bounds rules shared by VASource and VASourceLeech, which have no raytraced base class in common
struct VABoundsPlayback
{
    // Call at the top of play(). Returns whether to play now, otherwise the play is queued until it's ready, and a loop waits until it's inside the bounds. A sound that doesn't loop isn't played outside the bounds, with a warning
    bool request_play(ALSource &source, bool silenced, bool ready, float start_time);

    // Call every frame. While silenced it stops the sound, re-queues a loop that was playing so it resumes inside, and cancels a queued play that doesn't loop. Returns whether to call play(get_pending_start_time()) now, for a queued play or autoplay
    bool process(ALSource &source, bool silenced, bool ready, bool autoplay);

    void on_played()
    {
        play_pending = false;
        pending_start_time = 0.0f;
        autoplay_done = true;
    }

    // The time in seconds the queued play starts from
    float get_pending_start_time() const
    {
        return pending_start_time;
    }

    // stop() cancels any queued play, as if it had played
    void cancel()
    {
        on_played();
    }

private:
    // A play() that hasn't started yet: waiting for the emitter to be ready, a loop waiting for the bounds, or one the bounds stopped
    bool play_pending = false;
    float pending_start_time = 0.0f;

    // Set once played or cancelled, so autoplay only starts it once
    bool autoplay_done = false;

    // Returns whether it's silenced
    bool stop_if_silenced(ALSource &source, bool silenced);
};
