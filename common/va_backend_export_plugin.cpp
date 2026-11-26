// AI-created

#include "va_backend_export_plugin.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "backend/va_audio_backend_registry.h"

using namespace godot;

namespace va_godot
{

void VABackendExportPlugin::_export_begin(const PackedStringArray &features, bool is_debug, const String &path, uint32_t flags)
{
    if (!VAAudioBackendRegistry::is_selected("OpenAL"))
        return;

    String folder = ProjectSettings::get_singleton()->get_setting("audio/vaudio/openal_hrtf_folder");

    if (folder.is_empty() || !DirAccess::dir_exists_absolute(folder))
        return;

    PackedStringArray files = DirAccess::get_files_at(folder);

    for (int i = 0; i < files.size(); i++)
    {
        if (files[i].get_extension().to_lower() != "mhr")
            continue;

        String file_path = folder.path_join(files[i]);
        add_file(file_path, FileAccess::get_file_as_bytes(file_path), false);
    }
}

void VABackendExportPlugin::_export_file(const String &path, const String &type, const PackedStringArray &features)
{
    if (path.get_extension() != "gdextension" || !path.get_file().begins_with("vaudio_godot_native_openal"))
        return;

    // Same folder and file names as the .gdextension's [libraries] and SConstruct
    String folder;
    String openal_file;
    PackedStringArray debug_window_files;

    if (features.has("macos"))
    {
        folder = "mac";
        openal_file = "libopenal.1.dylib";
        debug_window_files = { "vaudio-debug-window", "libglfw.3.dylib" };
    }
    else if (features.has("windows"))
    {
        folder = "windows";
        openal_file = "soft_oal.dll";
        debug_window_files = { "vaudio-debug-window.exe", "glfw3.dll" };
    }
    else if (features.has("linux"))
    {
        folder = "linux";
        openal_file = "libopenal.so.1";
        debug_window_files = { "vaudio-debug-window", "libglfw.so.3" };
    }
    else
        return;

    if (folder != "mac" && features.has("arm64"))
        folder += "-arm64";

    String bin_dir = path.get_base_dir().path_join("bin").path_join(folder);

    // OpenAL Soft ships beside the plugin binary, where OpenALBackend loads it from, but only when it's the selected backend
    if (VAAudioBackendRegistry::is_selected("OpenAL"))
    {
        String openal_path = bin_dir.path_join(openal_file);

        if (FileAccess::file_exists(openal_path))
            add_shared_object(openal_path, PackedStringArray(), "");
        else
            UtilityFunctions::push_error("[vaudio] ", openal_path, " is missing, so the exported game will have no audio. Copy OpenAL Soft into ", ProjectSettings::get_singleton()->globalize_path(bin_dir));
    }

    for (const String &name : debug_window_files)
    {
        String file_path = bin_dir.path_join(name);

        if (FileAccess::file_exists(file_path))
            add_shared_object(file_path, PackedStringArray(), "");
    }
}

} // namespace va_godot
