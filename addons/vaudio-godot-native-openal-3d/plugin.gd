@tool
extends EditorPlugin

const LOG_TAG = "[vaudio-godot-native-openal-3d]"
const BIN_DIR = "res://addons/vaudio-godot-native-openal-3d/bin"

var main_plugin: EditorPlugin

func _enter_tree():
	var sdk_lib = _get_sdk_lib_name()
	var sdk_lib_path = BIN_DIR.path_join(sdk_lib)

	if sdk_lib != "" and not FileAccess.file_exists(sdk_lib_path):
		# Logged here rather than by the GDExtension, which loads before the editor Output panel exists
		push_error("%s %s not found - please copy the Vercidium Audio SDK's %s into %s, then restart Godot" % [LOG_TAG, sdk_lib, sdk_lib, ProjectSettings.globalize_path(BIN_DIR)])
		return

	# The GDExtension failed to load for some other reason - Godot's own errors above have the details
	if not ClassDB.class_exists("VAConversionPlugin"):
		push_error("%s The plugin's GDExtension failed to load - check the errors above, make sure every file in %s is the 64-bit build from the same vaudio version, then restart Godot" % [LOG_TAG, ProjectSettings.globalize_path(BIN_DIR)])
		return

	# VAConversionPlugin is the C++ EditorPlugin - adding it as a child runs its _enter_tree/_exit_tree alongside this one
	main_plugin = ClassDB.instantiate("VAConversionPlugin")
	add_child(main_plugin)

func _exit_tree():
	if main_plugin:
		remove_child(main_plugin)
		main_plugin.queue_free()
		main_plugin = null

func _get_sdk_lib_name() -> String:
	match OS.get_name():
		"Windows": return "vaudionative.dll"
		"Linux": return "libvaudionative.so"
		"macOS": return "libvaudionative.dylib"
	return ""
