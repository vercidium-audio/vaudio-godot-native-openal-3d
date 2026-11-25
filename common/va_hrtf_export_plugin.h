// AI-created

#pragma once

#include <godot_cpp/classes/editor_export_plugin.hpp>

using namespace godot;

namespace va_godot
{

// .mhr files aren't Godot resources, so exports skip them unless they're added to the include filter by hand. This bundles the audio/vaudio/hrtf_folder ones automatically, plus the dev SDK's debug window, which the .gdextension's [dependencies] can't list since production SDKs don't have it
class VAHRTFExportPlugin : public EditorExportPlugin
{
    GDCLASS(VAHRTFExportPlugin, EditorExportPlugin);

protected:
    static void _bind_methods() {}

public:
    String _get_name() const override
    {
        return "VAHRTFExportPlugin";
    }

    void _export_begin(const PackedStringArray &features, bool is_debug, const String &path, uint32_t flags) override;
    void _export_file(const String &path, const String &type, const PackedStringArray &features) override;
};

} // namespace va_godot
