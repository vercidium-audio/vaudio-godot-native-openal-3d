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

// "windows", "linux", "macos" or empty for an export platform the plugin doesn't support
static String feature_os(const PackedStringArray &features)
{
    for (const char *os : { "windows", "linux", "macos" })
    {
        if (features.has(os))
            return os;
    }

    return String();
}

// The names FMODLibrary loads at runtime
static String fmod_library_name(const String &os)
{
    if (os == "windows")
        return "fmod.dll";
    if (os == "linux")
        return "libfmod.so";
    if (os == "macos")
        return "libfmod.dylib";

    return String();
}

static void export_hrtf(VABackendExportPlugin *plugin)
{
    String folder = ProjectSettings::get_singleton()->get_setting("audio/vaudio/openal_hrtf_folder");

    if (folder.is_empty() || !DirAccess::dir_exists_absolute(folder))
        return;

    PackedStringArray files = DirAccess::get_files_at(folder);

    for (int i = 0; i < files.size(); i++)
    {
        if (files[i].get_extension().to_lower() != "mhr")
            continue;

        String file_path = folder.path_join(files[i]);
        plugin->add_file(file_path, FileAccess::get_file_as_bytes(file_path), false);
    }
}

// Looks in the project folder, then audio/vaudio/fmod_library_path (the library or its folder). The library must be the export target's platform and architecture
static void export_fmod(VABackendExportPlugin *plugin, const PackedStringArray &features)
{
    String library_name = fmod_library_name(feature_os(features));

    if (library_name.is_empty())
    {
        UtilityFunctions::push_warning("[vaudio] FMOD isn't supported on this export platform (", String(", ").join(features), "), so the exported game will have no audio");
        return;
    }

    ProjectSettings *settings = ProjectSettings::get_singleton();
    PackedStringArray candidates = { settings->globalize_path("res://" + library_name) };

    String setting = settings->get_setting("audio/vaudio/fmod_library_path", "");
    if (!setting.is_empty())
    {
        String path = settings->globalize_path(setting);
        candidates.push_back(DirAccess::dir_exists_absolute(path) ? path.path_join(library_name) : path);
    }

    for (const String &candidate : candidates)
    {
        if (FileAccess::file_exists(candidate))
        {
            plugin->add_shared_object(candidate, PackedStringArray(), "");
            return;
        }
    }

    UtilityFunctions::push_warning("[vaudio] The FMOD library ", library_name, " wasn't found, so it isn't in the export. Copy it from the FMOD Engine (FMOD Studio API) 2.03.x install beside the exported game's executable, or the game will have no audio. Searched: ", String(", ").join(candidates));
}

void VABackendExportPlugin::_export_begin(const PackedStringArray &features, bool is_debug, const String &path, uint32_t flags)
{
    if (VAAudioBackendRegistry::is_selected("OpenAL"))
        export_hrtf(this);
    else if (VAAudioBackendRegistry::is_selected("FMOD"))
        export_fmod(this, features);
}

void VABackendExportPlugin::_export_file(const String &path, const String &type, const PackedStringArray &features)
{
    if (path.get_extension() != "gdextension" || !path.get_file().begins_with("vaudio_godot_native_openal"))
        return;

    // Same folder and file names as the .gdextension's [libraries] and SConstruct
    String os = feature_os(features);
    String folder;
    String openal_file;
    PackedStringArray debug_window_files;

    if (os == "macos")
    {
        folder = "mac";
        openal_file = "libopenal.1.dylib";
        debug_window_files = { "vaudio-debug-window", "libglfw.3.dylib" };
    }
    else if (os == "windows")
    {
        folder = "windows";
        openal_file = "soft_oal.dll";
        debug_window_files = { "vaudio-debug-window.exe", "glfw3.dll" };
    }
    else if (os == "linux")
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
