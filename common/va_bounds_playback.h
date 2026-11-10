#pragma once

class ALSource;

// Autoplay and world bounds rules shared by VASource and VASourceLeech, which have no raytraced base class in common
struct VABoundsPlayback
{
    // A play() that hasn't started yet: a loop waiting for the bounds, or one the bounds stopped
    bool play_pending = false;

    // Set once played or cancelled, so autoplay only starts it once
    bool autoplay_done = false;

    bool is_pending(bool autoplay) const
    {
        return play_pending || (autoplay && !autoplay_done);
    }

    void on_played()
    {
        play_pending = false;
        autoplay_done = true;
    }

    // stop() cancels any waiting play, as if it had played
    void cancel()
    {
        on_played();
    }

    // Call every frame before autoplay. While silenced it stops the sound, re-arms a loop that was playing so it resumes inside, and cancels a pending play that doesn't loop. Returns whether it's silenced
    bool apply(ALSource &source, bool silenced);

    // Call at the top of play(). Returns whether the bounds stopped it from playing, warning for a sound that doesn't loop. A loop waits and plays once inside
    bool apply_on_play(ALSource &source, bool silenced);
};
