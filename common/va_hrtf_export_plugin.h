#pragma once

#include <godot_cpp/classes/editor_export_plugin.hpp>

using namespace godot;

namespace va_godot
{

// .mhr files aren't Godot resources, so exports skip them unless they're added to the include filter by hand. This bundles the audio/vaudio/hrtf_folder ones automatically
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
};

} // namespace va_godot
