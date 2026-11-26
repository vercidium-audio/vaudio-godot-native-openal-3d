// AI-created

#include "va_audio_backend_registry.h"

#include <godot_cpp/classes/project_settings.hpp>

#include <string>
#include <utility>
#include <vector>

using namespace godot;

// A function-local static, so backends registering from other files' static initialisers never run before it's constructed
static std::vector<std::pair<std::string, VAAudioBackendRegistry::Factory>> &get_factories()
{
    static std::vector<std::pair<std::string, VAAudioBackendRegistry::Factory>> factories;
    return factories;
}

static VAAudioBackendRegistry::Factory find_factory(const String &name)
{
    for (const auto &entry : get_factories())
    {
        if (name.nocasecmp_to(String::utf8(entry.first.c_str())) == 0)
            return entry.second;
    }

    return nullptr;
}

bool VAAudioBackendRegistry::register_backend(const char *name, Factory factory)
{
    get_factories().emplace_back(name, factory);
    return true;
}

PackedStringArray VAAudioBackendRegistry::get_names()
{
    PackedStringArray names;

    for (const auto &entry : get_factories())
        names.push_back(String::utf8(entry.first.c_str()));

    return names;
}

bool VAAudioBackendRegistry::is_selected(const char *name)
{
    String selected = ProjectSettings::get_singleton()->get_setting(SETTING_NAME, DEFAULT_BACKEND);
    return selected.nocasecmp_to(String::utf8(name)) == 0;
}

std::unique_ptr<VAAudioBackendBase> VAAudioBackendRegistry::create_selected()
{
    String selected = ProjectSettings::get_singleton()->get_setting(SETTING_NAME, DEFAULT_BACKEND);

    if (Factory factory = find_factory(selected))
        return std::unique_ptr<VAAudioBackendBase>(factory());

    String fallback = find_factory(DEFAULT_BACKEND) ? String(DEFAULT_BACKEND) : get_names().is_empty() ? String() : get_names()[0];

    if (fallback.is_empty())
    {
        VA_BACKEND_ERROR("No audio backends are available, so no audio will play. The '", selected, "' backend is selected in the ", SETTING_NAME, " project setting");
        return nullptr;
    }

    VA_BACKEND_ERROR("The '", selected, "' audio backend selected in the ", SETTING_NAME, " project setting is not available, so the ", fallback, " backend is used instead");
    return std::unique_ptr<VAAudioBackendBase>(find_factory(fallback)());
}
