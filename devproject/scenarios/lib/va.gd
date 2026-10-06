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

static func open_stream(source: Node, format: int, frequency: int) -> bool:
	return source.open_stream(format, frequency)

static func push_audio_data(source: Node, data: PackedByteArray) -> void:
	source.push_audio_data(data)

static func close_stream(source: Node) -> void:
	source.close_stream()

static func is_stream_open(source: Node) -> bool:
	return source.is_stream_open()

static func stop(source: Node) -> void:
	source.stop()
