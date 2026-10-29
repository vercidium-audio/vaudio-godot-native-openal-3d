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

} // namespace va_godot
