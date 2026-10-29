#pragma once

#ifndef _WIN32
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#endif

#include <vaudio.h>

#include "va_engine_util.h"

namespace va_godot
{

// The SDK only reports a non-executable vaudio-debug-window on stderr, which the Godot editor never shows - so check it here and push_warning instead. Missing is fine, production packages don't ship it.
inline void warn_if_debug_window_not_executable()
{
#ifndef _WIN32
    static bool checked = false;
    if (checked)
        return;

    checked = true;

    // The debug window sits beside the vaudionative shared library, which is where the SDK looks for it too
    Dl_info info;
    if (dladdr((void *)&vaWorldCreate, &info) == 0 || info.dli_fname == nullptr)
        return;

    std::string path = info.dli_fname;
    size_t last_slash = path.find_last_of('/');
    if (last_slash == std::string::npos)
        return;

    path = path.substr(0, last_slash) + "/vaudio-debug-window";

    struct stat st;
    if (stat(path.c_str(), &st) != 0 || access(path.c_str(), X_OK) == 0)
        return;

    VA_WARN("vaudio-debug-window is not executable, so the debug window can't open. Run: chmod +x \"", path.c_str(), "\"");
#endif
}

// vaWorldSetRenderingEnabled returns VA_FEATURE_DISABLED when the debug window failed to spawn at world creation, but the SDK only reports why on stderr
inline void error_if_debug_window_missing(VAResult result)
{
    if (result != VA_FEATURE_DISABLED)
        return;

#ifdef _WIN32
    VA_ERROR("Failed to open the debug window. Make sure vaudio-debug-window.exe and glfw3.dll from the vaudio dev build are copied beside vaudionative.dll in the plugin's bin folder, and then restart the game.");
#else
    VA_ERROR("Failed to open the debug window. Make sure vaudio-debug-window from the vaudio dev build is copied beside the vaudionative library in the plugin's bin folder, and is executable, and then restart the game.");
#endif
}

} // namespace va_godot
