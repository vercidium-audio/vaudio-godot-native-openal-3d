#include "va_hrtf_export_plugin.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>

using namespace godot;

namespace va_godot
{

void VAHRTFExportPlugin::_export_begin(const PackedStringArray &features, bool is_debug, const String &path, uint32_t flags)
{
    String folder = ProjectSettings::get_singleton()->get_setting("audio/vaudio/hrtf_folder");

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

void VAHRTFExportPlugin::_export_file(const String &path, const String &type, const PackedStringArray &features)
{
    if (path.get_extension() != "gdextension" || !path.get_file().begins_with("vaudio_godot_native_openal"))
        return;

    // Same folder names as the .gdextension's [libraries] and SConstruct
    String folder;
    PackedStringArray debug_window_files;

    if (features.has("macos"))
    {
        folder = "mac";
        debug_window_files = { "vaudio-debug-window", "libglfw.3.dylib" };
    }
    else if (features.has("windows"))
    {
        folder = "windows";
        debug_window_files = { "vaudio-debug-window.exe", "glfw3.dll" };
    }
    else if (features.has("linux"))
    {
        folder = "linux";
        debug_window_files = { "vaudio-debug-window", "libglfw.so.3" };
    }
    else
        return;

    if (folder != "mac" && features.has("arm64"))
        folder += "-arm64";

    String bin_dir = path.get_base_dir().path_join("bin").path_join(folder);

    for (const String &name : debug_window_files)
    {
        String file_path = bin_dir.path_join(name);

        if (FileAccess::file_exists(file_path))
            add_shared_object(file_path, PackedStringArray(), "");
    }
}

} // namespace va_godot
