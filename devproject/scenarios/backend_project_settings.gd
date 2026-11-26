# AI-created

extends "res://scenarios/lib/scenario.gd"

# The native plugin registers the audio/vaudio/* settings at startup (the C# plugin only does in the editor): audio/vaudio/backend selects the running backend, the OpenAL-only settings have the openal_ prefix (the 1.11 names are gone), the FMOD ones fmod_, and only the selected backend's are shown in Project Settings

const OPENAL_SETTINGS := ["openal_hrtf_enabled", "openal_hrtf", "openal_hrtf_folder", "openal_max_reverb_sends", "openal_max_mono_sources", "openal_max_stereo_sources"]
const FMOD_SETTINGS := ["fmod_library_path", "fmod_max_channels", "fmod_dsp_buffer_size"]
const SHARED_SETTINGS := ["backend", "output_device", "sample_rate"]
const REMOVED_SETTINGS := ["hrtf_enabled", "hrtf", "hrtf_folder", "max_reverb_sends", "max_mono_sources", "max_stereo_sources"]

func run() -> void:
	var selected: String = ProjectSettings.get_setting("audio/vaudio/backend")
	check(VA.backend_name(root.world) == selected, "the running backend is '%s', but audio/vaudio/backend is '%s'" % [VA.backend_name(root.world), selected])
	check(VA.is_backend_initialised(root.world), "the %s backend didn't initialise" % selected)

	for setting in SHARED_SETTINGS:
		check(is_shown("audio/vaudio/" + setting), "audio/vaudio/%s isn't registered and shown" % setting)

	for setting in REMOVED_SETTINGS:
		check(not ProjectSettings.has_setting("audio/vaudio/" + setting), "the 1.11 setting audio/vaudio/%s is still registered" % setting)

	for setting in OPENAL_SETTINGS:
		check(is_shown("audio/vaudio/" + setting) == (selected == "OpenAL"), "audio/vaudio/%s should be shown only for OpenAL (running on %s)" % [setting, selected])

	for setting in FMOD_SETTINGS:
		check(is_shown("audio/vaudio/" + setting) == (selected == "FMOD"), "audio/vaudio/%s should be shown only for FMOD (running on %s)" % [setting, selected])

# Registered, and not hidden by set_as_internal
func is_shown(setting: String) -> bool:
	for property in ProjectSettings.get_property_list():
		if property.name == setting:
			return property.usage & PROPERTY_USAGE_EDITOR != 0
	return false
