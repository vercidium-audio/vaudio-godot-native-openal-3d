// AI-created

#pragma once

#include <godot_cpp/classes/editor_export_plugin.hpp>

using namespace godot;

namespace va_godot
{

// Adds the export files the .gdextension can't list: the selected backend's library (OpenAL: the addon's OpenAL Soft plus the audio/vaudio/openal_hrtf_folder .mhr files, which aren't Godot resources, so exports skip them unless they're added to the include filter by hand), plus the dev SDK's debug window, which production SDKs don't have
class VABackendExportPlugin : public EditorExportPlugin
{
    GDCLASS(VABackendExportPlugin, EditorExportPlugin);

protected:
    static void _bind_methods() {}

public:
    String _get_name() const override
    {
        return "VABackendExportPlugin";
    }

    void _export_begin(const PackedStringArray &features, bool is_debug, const String &path, uint32_t flags) override;
    void _export_file(const String &path, const String &type, const PackedStringArray &features) override;
};

} // namespace va_godot
