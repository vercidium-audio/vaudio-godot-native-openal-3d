# AI-created

extends RefCounted

# Standard (native GDExtension) plugin adapter - the C# plugin has its own va.gd with the same functions. Property names are always given in snake_case.

static func get_value(node: Object, property: String) -> Variant:
	if not property in node:
		push_error("[devproject] %s has no property '%s'" % [node, property])
		return null
	return node.get(property)

static func set_value(node: Object, property: String, value: Variant) -> void:
	if not property in node:
		push_error("[devproject] %s has no property '%s'" % [node, property])
		return
	node.set(property, value)

static func connect_signal(node: Object, signal_name: String, callable: Callable) -> void:
	if not node.has_signal(signal_name):
		push_error("[devproject] %s has no signal '%s'" % [node, signal_name])
		return
	node.connect(signal_name, callable)

# The property's info from get_property_list(), after the node's _validate_property, or empty if it has none
static func property_info(node: Object, property: String) -> Dictionary:
	for info in node.get_property_list():
		if info.name == property:
			return info
	return {}

# Whether the property is shown in the inspector, after the node's _validate_property
static func is_in_inspector(node: Object, property: String) -> bool:
	return property_info(node, property).get("usage", 0) & PROPERTY_USAGE_EDITOR != 0

# Creates a plugin node by class name, e.g. "VADefaultMaterial". The world is unused here, but the C# adapter needs it to find the addon folder
static func create_node(_world: Node, type: String) -> Node:
	return ClassDB.instantiate(type)

static func is_raytraced(source: Node) -> bool:
	return source.is_raytraced()

static func is_raytraced_by_listener(source: Node) -> bool:
	return source.is_raytraced_by_listener()

static func muffling_lf(source: Node) -> float:
	return source.get_muffling_gain_lf()

static func muffling_hf(source: Node) -> float:
	return source.get_muffling_gain_hf()

static func grouped_eax_index(source: Node) -> int:
	return source.get_grouped_eax_index()

static func raytrace_count(world: Node) -> int:
	return world.get_raytrace_count()

static func bounds_position(world: Node) -> Variant:
	return world.get_bounds_position()

static func is_ready_to_play(source: Node) -> bool:
	return source.is_ready_to_play()

static func is_within_bounds(world: Node, position) -> bool:
	return world.is_within_bounds(position)

static func is_silenced_by_bounds(source: Node) -> bool:
	return source.is_silenced_by_bounds()

# Whether the source's sound is sent to a reverb effect (grouped or the listener's)
static func has_reverb(source: Node) -> bool:
	return source.has_reverb()

static func is_dry_output_enabled(source: Node) -> bool:
	return source.is_dry_output_enabled()

static func raytracing_time(world: Node) -> float:
	return world.get_raytracing_time()

static func grouped_eax_count(world: Node) -> int:
	return world.get_grouped_eax_count()

static func grouped_eax_decay_time(world: Node, index: int) -> float:
	return world.get_grouped_eax_decay_time(index)

static func grouped_eax_gain_lf(world: Node, index: int) -> float:
	return world.get_grouped_eax_gain_lf(index)

static func grouped_eax_gain_hf(world: Node, index: int) -> float:
	return world.get_grouped_eax_gain_hf(index)

static func export_to_file(world: Node, path: String) -> bool:
	return world.export_to_file(path)

static func sync_primitive(world: Node, node: Node) -> void:
	world.sync_primitive(node)

# Decay time of the grouped reverb this source contributes to, or -1 if it isn't grouped yet
static func decay_time(world: Node, source: Node) -> float:
	var index := grouped_eax_index(source)
	if index < 0 or index >= grouped_eax_count(world):
		return -1.0
	return grouped_eax_decay_time(world, index)

static func is_playing(source: Node) -> bool:
	return source.is_playing()

static func make_current(listener: Node) -> void:
	listener.make_current()

# VAPcmFormat values for open_stream(), open_capture() and VAInputStreamSource's format
const PCM_MONO16 := 1
const PCM_MONO_FLOAT := 4

static func open_stream(source: Node, format: int, frequency: int) -> bool:
	return source.open_stream(format, frequency)

static func push_audio_data(source: Node, data: PackedByteArray) -> void:
	source.push_audio_data(data)

static func open_capture(source: Node, device_name: String, format: int, frequency: int, buffer_size_frames: int) -> bool:
	return source.open_capture(device_name, format, frequency, buffer_size_frames)

static func is_capture_open(source: Node) -> bool:
	return source.is_capture_open()

static func start_capture(source: Node) -> void:
	source.start_capture()

static func stop_capture(source: Node) -> void:
	source.stop_capture()

static func close_capture(source: Node) -> void:
	source.close_capture()

# What the inspector's Refresh Audio Devices button does: re-queries the backend's capture devices for device_name's dropdown
static func refresh_capture_devices(source: Node) -> void:
	source.refresh_devices()

static func close_stream(source: Node) -> void:
	source.close_stream()

static func is_stream_open(source: Node) -> bool:
	return source.is_stream_open()

static func play(source: Node) -> bool:
	return source.play()

static func stop(source: Node) -> void:
	source.stop()

static func play_from(source: Node, start_time: float) -> bool:
	return source.play(start_time)

static func set_stream_paused(source: Node, paused: bool) -> void:
	source.stream_paused = paused

static func is_stream_paused(source: Node) -> bool:
	return source.stream_paused

static func seek(source: Node, time: float) -> void:
	source.seek(time)

static func playback_time(source: Node) -> float:
	return source.get_playback_time()

# HRTF lives on the VAAudioBackend singleton here. The world is unused, but the C# adapter needs it to reach VAAudioBackend
static func available_hrtfs(_world: Node) -> PackedStringArray:
	return VAAudioBackend.get_available_hrtfs()

static func set_hrtf(_world: Node, hrtf_name: String) -> bool:
	return VAAudioBackend.set_hrtf(hrtf_name)

static func get_hrtf(_world: Node) -> String:
	return VAAudioBackend.get_hrtf()

static func active_hrtf(_world: Node) -> String:
	return VAAudioBackend.get_active_hrtf()

static func set_hrtf_enabled(_world: Node, value: bool) -> bool:
	return VAAudioBackend.set_hrtf_enabled(value)

# How many errors the audio backend has logged since the game started. The world is unused, but the C# adapter needs it to reach VAAudioBackend
static func backend_error_count(_world: Node) -> int:
	return VAAudioBackend.get_error_count()

static func backend_name(_world: Node) -> String:
	return VAAudioBackend.get_backend_name()

static func is_backend_initialised(_world: Node) -> bool:
	return VAAudioBackend.is_initialized()

# The global mix settings the audio backend is using, keyed by the VAWorld property they come from, e.g. "doppler_scale". The world is unused, but the C# adapter needs it to reach the backend
static func backend_settings(_world: Node) -> Dictionary:
	var settings := {}
	for property in ["master_volume", "distance_model", "doppler_scale", "meters_per_unit", "speed_of_sound", "reverb_only"]:
		settings[property] = VAAudioBackend.get(property)
	return settings

# The reverb send level (0-1) of a player's most recent voice after distance attenuation, or -1 when it isn't sending or the backend doesn't report it
static func reverb_send_level(_world: Node, player: Node) -> float:
	return VAAudioBackend.get_reverb_send_level(player)
