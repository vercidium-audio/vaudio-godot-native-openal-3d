// AI-created

#pragma once

#include <godot_cpp/variant/packed_string_array.hpp>

#include <memory>

#include "va_audio_backend_base.h"

// The audio backends compiled into this plugin. Each backend registers itself from a static initialiser in its own folder, so core code never names a backend type
class VAAudioBackendRegistry
{
public:
    static constexpr const char *SETTING_NAME = "audio/vaudio/backend";
    static constexpr const char *DEFAULT_BACKEND = "OpenAL";

    using Factory = VAAudioBackendBase *(*)();

    // Safe to call during static initialisation, since it touches no Godot types
    static bool register_backend(const char *name, Factory factory);

    static godot::PackedStringArray get_names();

    // Whether audio/vaudio/backend names this backend (case-insensitive), whether or not it's compiled in
    static bool is_selected(const char *name);

    // Creates the backend chosen by audio/vaudio/backend, falling back to the default (or any registered one) with a counted error when it isn't available. nullptr if none are registered
    static std::unique_ptr<VAAudioBackendBase> create_selected();
};
