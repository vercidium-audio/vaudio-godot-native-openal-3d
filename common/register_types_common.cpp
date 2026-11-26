// AI-created

#include "register_types.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "backend/va_audio_backend_registry.h"
#include "va_custom_material.h"
#include "va_default_material.h"
#include "va_device_name.h"
#include "va_engine_util.h"
#include "va_world.h"
#include "va_world_lookup.h"

#include <string>

using namespace godot;

Node *find_child_named_recursive(Node *node, const StringName &name)
{
    TypedArray<Node> children = node->get_children();

    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);

        if (child->get_name() == name)
            return child;

        if (Node *found = find_child_named_recursive(child, name))
            return found;
    }

    return nullptr;
}

static Node *replace_with_material_node(Node *scene_root, const NodePath &node_path, Node *existing_node, const String &node_name,
    bool is_custom_material, int material_type)
{
    Node *parent;

    if (existing_node)
    {
        parent = existing_node->get_parent();

        if (parent)
            parent->remove_child(existing_node);

        existing_node->queue_free();
    }
    else
    {
        int64_t name_count = node_path.get_name_count();
        if (name_count < 2)
            return nullptr;

        parent = scene_root->get_node_or_null(node_path.slice(0, name_count - 1));
    }

    if (!Object::cast_to<va_godot::VAWorld>(parent))
        return nullptr;

    Node *node;

    if (is_custom_material)
    {
        node = memnew(va_godot::VACustomMaterial);
    }
    else
    {
        VADefaultMaterial *default_material = memnew(VADefaultMaterial);
        default_material->set_material_type(material_type);
        node = default_material;
    }

    node->set_name(node_name);
    parent->add_child(node);

    return node;
}

static bool on_sync_material_properties(const Array &data)
{
    // scene_root_name, node_path, node_name, is_custom_material, material_type, 7 material floats, color
    if (data.size() < 13)
        return false;

    SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    Node *tree_root = scene_tree ? scene_tree->get_root() : nullptr;

    if (!tree_root)
    {
        VA_WARN("Received a material property edit from the editor, but the game has no scene tree root");
        return true;
    }

    StringName scene_root_name = data[0];
    Node *scene_root = find_child_named_recursive(tree_root, scene_root_name);

    if (!scene_root)
    {
        VA_WARN("Received a material property edit from the editor, but no node named '", scene_root_name, "' exists in the running scene");
        return true;
    }

    NodePath node_path = data[1];
    Node *node = scene_root->get_node_or_null(node_path);

    String node_name = data[2];
    bool is_custom_material = data[3];
    int material_type = data[4];

    float absorption_lf = data[5];
    float absorption_hf = data[6];
    float scattering = data[7];
    float transmission_lf = data[8];
    float transmission_hf = data[9];
    float flat_transmission_lf = data[10];
    float flat_transmission_hf = data[11];
    Color color = data[12];

    if (!Object::cast_to<VADefaultMaterial>(node) && !Object::cast_to<va_godot::VACustomMaterial>(node))
        node = replace_with_material_node(scene_root, node_path, node, node_name, is_custom_material, material_type);

    if (VADefaultMaterial *default_material = Object::cast_to<VADefaultMaterial>(node))
        default_material->apply_properties_from_editor(absorption_lf, absorption_hf, scattering,
            transmission_lf, transmission_hf, flat_transmission_lf, flat_transmission_hf, color);
    else if (va_godot::VACustomMaterial *custom_material = Object::cast_to<va_godot::VACustomMaterial>(node))
        custom_material->apply_properties_from_editor(absorption_lf, absorption_hf, scattering,
            transmission_lf, transmission_hf, flat_transmission_lf, flat_transmission_hf, color);
    else
        VA_WARN(
            "Received a material property edit from the editor for '", node_path,
            "', but no matching VADefaultMaterial/VACustomMaterial node exists under '", scene_root_name, "' (", scene_root->get_path(),
            "), and its parent VAWorld node doesn't exist in the running scene either - restart the running game to pick it up");

    return true;
}

bool on_debugger_message(const String &message, const Array &data)
{
    if (message == "sync_material_properties")
        return on_sync_material_properties(data);

    if (message == "sync_viewport_camera")
        return on_sync_viewport_camera(data);

    if (message != "sync_primitive" || data.size() < 4)
        return false;

    SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    Node *tree_root = scene_tree ? scene_tree->get_root() : nullptr;

    if (!tree_root)
    {
        VA_WARN("Received a material/permeation edit from the editor, but the game has no scene tree root");
        return true;
    }

    StringName scene_root_name = data[0];
    Node *scene_root = find_child_named_recursive(tree_root, scene_root_name);

    if (!scene_root)
    {
        VA_WARN("Received a material/permeation edit from the editor, but no node named '", scene_root_name, "' exists in the running scene");
        return true;
    }

    NodePath node_path = data[1];
    Node *node = scene_root->get_node_or_null(node_path);

    if (!node)
    {
        VA_WARN(
            "Received a material/permeation edit from the editor for '", node_path,
            "', but no matching node exists under '", scene_root_name, "' (", scene_root->get_path(), ")");
        return true;
    }

    va_godot::VAWorld *va_world = va_godot::find_va_world(node);

    if (!va_world)
    {
        VA_WARN("Received a material/permeation edit from the editor for '", node->get_name(), "', but couldn't find a VAWorld in the running scene");
        return true;
    }

    // The running game has its own separate copy of this node - apply the metadata the editor's local copy just had
    // set/removed on it before re-adding the primitive below, since add_primitive/get_material read it straight off this node.
    String material = data[2];
    StringName material_meta_key = va_godot::VAWorld::get_material_meta_key();

    if (material.is_empty())
        node->remove_meta(material_meta_key);
    else
        node->set_meta(material_meta_key, material);

    Variant use_flat_transmission = data[3];
    StringName use_flat_transmission_meta_key = va_godot::VAWorld::get_use_flat_transmission_meta_key();

    if (use_flat_transmission.get_type() == Variant::NIL)
        node->remove_meta(use_flat_transmission_meta_key);
    else
        node->set_meta(use_flat_transmission_meta_key, use_flat_transmission);

    // Propagation filter (optional - older editor builds send a 4-element payload).
    if (data.size() > 4)
    {
        String propagate = data[4];
        StringName propagate_meta_key = va_godot::VAWorld::get_propagate_meta_key();

        if (propagate.is_empty())
            node->remove_meta(propagate_meta_key);
        else
            node->set_meta(propagate_meta_key, propagate);
    }

    va_world->sync_primitive(node);

    return true;
}

// Settings that only apply to one audio backend start with its prefix, and are hidden while another backend is selected. Matches the C# plugin's VAProjectSettings.gd
static const char *BACKEND_PREFIXES[][2] = {
    {"OpenAL", "audio/vaudio/openal_"},
    {"FMOD", "audio/vaudio/fmod_"},
};

// The backend the visible settings were last picked for. A std::string, since a static godot::String can't be constructed before the extension is initialised
static std::string visible_backend;

static void add_setting(const String &name, const Variant &default_value, Variant::Type type, PropertyHint hint = PROPERTY_HINT_NONE, const String &hint_string = String())
{
    ProjectSettings *settings = ProjectSettings::get_singleton();

    if (!settings->has_setting(name))
        settings->set_setting(name, default_value);

    settings->set_initial_value(name, default_value);

    Dictionary info;
    info["name"] = name;
    info["type"] = type;
    info["hint"] = hint;
    info["hint_string"] = hint_string;
    settings->add_property_info(info);
}

void update_backend_visibility()
{
    ProjectSettings *settings = ProjectSettings::get_singleton();
    String selected = settings->get_setting(VAAudioBackendRegistry::SETTING_NAME, VAAudioBackendRegistry::DEFAULT_BACKEND);
    std::string selected_utf8 = selected.utf8().get_data();

    if (selected_utf8 == visible_backend)
        return;

    visible_backend = selected_utf8;

    TypedArray<Dictionary> properties = settings->get_property_list();

    for (int i = 0; i < properties.size(); i++)
    {
        String name = Dictionary(properties[i])["name"];

        for (const auto &prefix : BACKEND_PREFIXES)
        {
            // Internal settings keep their saved values in project.godot, so switching back shows them unchanged
            if (name.begins_with(prefix[1]))
                settings->set_as_internal(name, selected != prefix[0]);
        }
    }
}

// Registers this plugin's own audio/vaudio/* entries under Project Settings. They must exist before VAAudioBackend::initialize() reads them to open the one-and-only device
void register_project_settings()
{
    ProjectSettings *settings = ProjectSettings::get_singleton();

    // Each backend's library is loaded once at startup
    PackedStringArray backend_names;

    for (const auto &prefix : BACKEND_PREFIXES)
        backend_names.push_back(prefix[0]);

    add_setting(VAAudioBackendRegistry::SETTING_NAME, VAAudioBackendRegistry::DEFAULT_BACKEND, Variant::STRING, PROPERTY_HINT_ENUM, String(",").join(backend_names));
    settings->set_restart_if_changed(VAAudioBackendRegistry::SETTING_NAME, true);

    // Stored as DEFAULT_DEVICE_LABEL, not "", so the strict PROPERTY_HINT_ENUM dropdown always has a current value among its own entries. The backend translates it back to "" ("driver default"). The dropdown is filled by VAAudioBackend::refresh_device_lists()
    add_setting("audio/vaudio/output_device", DEFAULT_DEVICE_LABEL, Variant::STRING, PROPERTY_HINT_ENUM, DEFAULT_DEVICE_LABEL);

    // 0 means the driver default
    add_setting("audio/vaudio/sample_rate", 0, Variant::INT, PROPERTY_HINT_ENUM, "System Default:0,22050,44100,48000,96000");

    // Dev-only setting (not end-user-facing)
    add_setting("audio/vaudio/openal_max_reverb_sends", 1, Variant::INT, PROPERTY_HINT_RANGE, "1,16,or_greater");

    add_setting("audio/vaudio/openal_hrtf_enabled", true, Variant::BOOL);

    // Same DEFAULT_*_LABEL storage as output_device. The dropdown is filled by VAAudioBackend::refresh_device_lists() once OpenAL has a device to enumerate HRTFs from
    add_setting("audio/vaudio/openal_hrtf", DEFAULT_HRTF_LABEL, Variant::STRING, PROPERTY_HINT_ENUM, DEFAULT_HRTF_LABEL);

    // .mhr files in here are listed alongside OpenAL Soft's built-in HRTF, and are bundled into exports by VABackendExportPlugin
    add_setting("audio/vaudio/openal_hrtf_folder", "res://hrtf", Variant::STRING, PROPERTY_HINT_DIR);

    // Read once before the one-and-only device open, not settable at runtime
    add_setting("audio/vaudio/openal_max_mono_sources", 16, Variant::INT, PROPERTY_HINT_RANGE, "0,256,or_greater");
    add_setting("audio/vaudio/openal_max_stereo_sources", 240, Variant::INT, PROPERTY_HINT_RANGE, "0,256,or_greater");

    // The FMOD library (or the folder holding it), searched after the project folder and the executable's folder. FMOD isn't shipped with vaudio
    add_setting("audio/vaudio/fmod_library_path", "", Variant::STRING, PROPERTY_HINT_GLOBAL_DIR);

    // Read once when FMOD initialises: the most sounds FMOD tracks at once (virtual voices)
    add_setting("audio/vaudio/fmod_max_channels", 512, Variant::INT, PROPERTY_HINT_RANGE, "1,4095");

    // Samples per mix buffer, 0 means FMOD's default (1024). Smaller is lower latency but uses more CPU
    add_setting("audio/vaudio/fmod_dsp_buffer_size", 0, Variant::INT, PROPERTY_HINT_ENUM, "FMOD Default:0,256,512,1024,2048");

    update_backend_visibility();

    settings->connect("settings_changed", callable_mp_static(&update_backend_visibility));
}

void unregister_project_settings()
{
    ProjectSettings *settings = ProjectSettings::get_singleton();
    Callable callable = callable_mp_static(&update_backend_visibility);

    if (settings->is_connected("settings_changed", callable))
        settings->disconnect("settings_changed", callable);

    visible_backend.clear();
}
