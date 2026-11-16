@tool
extends EditorPlugin

const LOG_TAG = "[vaudio-godot-native-openal-3d]"
const BIN_DIR = "res://addons/vaudio-godot-native-openal-3d/bin"

var main_plugin: EditorPlugin

func _enter_tree():
	var sdk_lib = _get_sdk_lib_name()
	var platform_dir = BIN_DIR.path_join(_get_platform_folder())
	var sdk_lib_path = platform_dir.path_join(sdk_lib)

	if sdk_lib != "" and not FileAccess.file_exists(sdk_lib_path):
		# Logged here rather than by the GDExtension, which loads before the editor Output panel exists
		push_error("%s %s not found - please copy the Vercidium Audio SDK's %s into %s, then restart Godot" % [LOG_TAG, sdk_lib, sdk_lib, ProjectSettings.globalize_path(platform_dir)])
		return

	# The GDExtension failed to load for some other reason - Godot's own errors above have the details
	if not ClassDB.class_exists("VAConversionPlugin"):
		push_error("%s The plugin's GDExtension failed to load - check the errors above, make sure every file in %s is from the same vaudio version and matches this platform, then restart Godot" % [LOG_TAG, ProjectSettings.globalize_path(platform_dir)])
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

# Same folder names as the vaudio SDK package and the .gdextension: windows, windows-arm64, linux, linux-arm64, mac
func _get_platform_folder() -> String:
	if OS.get_name() == "macOS":
		return "mac"
	var arch_suffix = "" if Engine.get_architecture_name() == "x86_64" else "-" + Engine.get_architecture_name()
	return OS.get_name().to_lower() + arch_suffix
