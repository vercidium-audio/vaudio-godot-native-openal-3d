# AI-created

extends "res://scenarios/lib/scenario.gd"

# 1.12 renamed the native player classes and the backend singleton to backend-neutral names (same as the C# plugin): ALSource/ALSource3D/ALSource2D -> VAPlayer/VAPlayer3D/VAPlayer2D, ALManager -> VAAudioBackend, and VAWorld's "OpenAL" inspector group -> "Audio Backend"

const REMOVED_CLASSES := ["ALSource", "ALSource3D", "ALSource2D", "ALSourceRelative", "ALManager"]

func run() -> void:
	var spatial := "VAPlayer2D" if root.world is Node2D else "VAPlayer3D"

	for name in ["VAPlayer", spatial, "VAAudioBackend"]:
		check(ClassDB.class_exists(name), "class %s isn't registered" % name)

	for name in REMOVED_CLASSES:
		check(not ClassDB.class_exists(name), "the 1.11 class %s is still registered" % name)

	check(Engine.has_singleton("VAAudioBackend"), "the VAAudioBackend singleton isn't registered")
	check(not Engine.has_singleton("ALManager"), "the 1.11 ALManager singleton is still registered")

	check(ClassDB.get_parent_class(spatial) == "VAPlayer", "%s should inherit VAPlayer, not %s" % [spatial, ClassDB.get_parent_class(spatial)])
	for source in ["VASource", "VASourceLeech", "VAStreamSource"]:
		check(ClassDB.is_parent_class(source, spatial), "%s should inherit %s" % [source, spatial])
	for source in ["VASourceRelative", "VASourceAmbient"]:
		check(ClassDB.get_parent_class(source) == "VAPlayer", "%s should inherit VAPlayer, not %s" % [source, ClassDB.get_parent_class(source)])

	var groups := []
	for property in root.world.get_property_list():
		if property.usage & PROPERTY_USAGE_GROUP:
			groups.append(property.name)
	check(groups.has("Audio Backend"), "VAWorld has no \"Audio Backend\" inspector group (groups: %s)" % ", ".join(groups))
	check(not groups.has("OpenAL"), "VAWorld still has the 1.11 \"OpenAL\" inspector group")
