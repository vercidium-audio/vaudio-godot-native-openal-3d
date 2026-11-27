// AI-created

#include "fmod_library.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include "backend/va_dynamic_library.h"
#include "fmod_api.h"

#include <string>

using namespace godot;

#ifdef _WIN32
static const char *FMOD_LIBRARY_NAME = "fmod.dll";
static const char *FMOD_LOGGING_LIBRARY_NAME = "fmodL.dll";
#elif defined(__APPLE__)
static const char *FMOD_LIBRARY_NAME = "libfmod.dylib";
static const char *FMOD_LOGGING_LIBRARY_NAME = "libfmodL.dylib";
#else
static const char *FMOD_LIBRARY_NAME = "libfmod.so";
static const char *FMOD_LOGGING_LIBRARY_NAME = "libfmodL.so";
#endif

static va_dynamic_library library = nullptr;

// UTF-8, since a static godot::String can't be constructed before the extension is initialised
static std::string loaded_from_utf8;

static PackedStringArray candidate_paths(const String &library_path_setting)
{
    PackedStringArray paths;
    String file_name = FMOD_LIBRARY_NAME;

    // The project folder, when running from the editor or `godot --path`
    if (!OS::get_singleton()->has_feature("template"))
        paths.push_back(ProjectSettings::get_singleton()->globalize_path("res://").path_join(file_name));

    paths.push_back(OS::get_singleton()->get_executable_path().get_base_dir().path_join(file_name));

    if (!library_path_setting.is_empty())
    {
        // Either the library itself or the folder holding it
        String path = ProjectSettings::get_singleton()->globalize_path(library_path_setting);
        paths.push_back(DirAccess::dir_exists_absolute(path) ? path.path_join(file_name) : path);
    }

    return paths;
}

// Another FMOD integration (or an earlier call) already loaded it, so share that runtime
static va_dynamic_library find_loaded()
{
    if (va_dynamic_library loaded = va_find_loaded_library(FMOD_LIBRARY_NAME))
        return loaded;

    return va_find_loaded_library(FMOD_LOGGING_LIBRARY_NAME);
}

static bool resolve_functions(va_dynamic_library from, const String &loaded_from, String &out_error)
{
    FMODApi api;
    PackedStringArray missing;

#define VA_FMOD_RESOLVE(name) \
    api.name = reinterpret_cast<decltype(api.name)>(va_library_symbol(from, #name)); \
    if (!api.name) \
        missing.push_back(#name);

    VA_FMOD_FUNCTIONS(VA_FMOD_RESOLVE)
#undef VA_FMOD_RESOLVE

    if (!missing.is_empty())
    {
        out_error = "The FMOD library loaded from " + loaded_from + " is missing " + String(", ").join(missing) + ". Use an FMOD Engine 2.03.x library";
        return false;
    }

    va_fmod = api;
    return true;
}

String FMODLibrary::load(const String &library_path_setting, String &out_error)
{
    if (library)
        return String::utf8(loaded_from_utf8.c_str());

    String loaded_from;
    va_dynamic_library found = find_loaded();

    if (found)
        loaded_from = "the already loaded module";

    PackedStringArray searched;

    if (!found)
    {
        for (const String &path : candidate_paths(library_path_setting))
        {
            searched.push_back(path);

            if (FileAccess::file_exists(path) && (found = va_open_library(path.utf8().get_data())))
            {
                loaded_from = path;
                break;
            }
        }
    }

    // The OS search (PATH etc.)
    if (!found && (found = va_open_library(FMOD_LIBRARY_NAME)))
        loaded_from = String("the system search path (") + FMOD_LIBRARY_NAME + ")";

    if (!found)
    {
        out_error = String("Failed to load the FMOD library ") + FMOD_LIBRARY_NAME + ", so the FMOD backend can't play audio. FMOD isn't shipped with vaudio: copy " + FMOD_LIBRARY_NAME + " from the FMOD Engine (FMOD Studio API) 2.03.x install into the project folder or beside the game's executable, or set audio/vaudio/fmod_library_path. Searched: " + String(", ").join(searched);
        return String();
    }

    if (!resolve_functions(found, loaded_from, out_error))
        return String();

    library = found;
    loaded_from_utf8 = loaded_from.utf8().get_data();
    return loaded_from;
}
