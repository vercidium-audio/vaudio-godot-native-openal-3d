// AI-created

#pragma once

#include <godot_cpp/variant/string.hpp>

// Finds the user-supplied FMOD Core library and resolves va_fmod from it. FMOD isn't shipped with vaudio (licence), so the plugin loads without it
class FMODLibrary
{
public:
    // Loads once and stays loaded. Returns where FMOD was loaded from, or an empty string with the reason (listing every path searched) in out_error
    static godot::String load(const godot::String &library_path_setting, godot::String &out_error);
};
