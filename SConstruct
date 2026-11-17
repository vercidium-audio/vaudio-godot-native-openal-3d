#!/usr/bin/env python
import os

env = SConscript("extern/godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

env.Append(CPPPATH=["src/", "common/", "thirdparty/vaudio/include/", "thirdparty/openal/include/"])

# Object files go in obj/ instead of beside the sources, since the 2D plugin's common/ is a junction to this one and both would write the same object file names into it
VariantDir("obj/src", "src", duplicate=0)
VariantDir("obj/common", "common", duplicate=0)
sources = Glob("obj/src/*.cpp") + Glob("obj/src/openal/*.cpp") + Glob("obj/common/*.cpp") + Glob("obj/common/openal/*.cpp")

if env["target"] in ["editor", "template_debug"]:
    doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
    sources.append(doc_data)

platform = env["platform"]
arch = env["arch"]

# Runtime file names per platform: vaudionative, OpenAL Soft, the dev-only debug window and the GLFW library it loads
if platform == "windows":
    vaudio_runtime_lib = "vaudionative.dll"
    openal_runtime_lib = "soft_oal.dll"
    debug_window_files = ["vaudio-debug-window.exe", "glfw3.dll"]
elif platform == "linux":
    vaudio_runtime_lib = "libvaudionative.so"
    openal_runtime_lib = "libopenal.so.1"
    debug_window_files = ["vaudio-debug-window", "libglfw.so.3"]
elif platform == "macos":
    vaudio_runtime_lib = "libvaudionative.dylib"
    openal_runtime_lib = "libopenal.1.dylib"
    debug_window_files = ["vaudio-debug-window", "libglfw.3.dylib"]
else:
    raise SCons.Errors.UserError(
        "vaudio-godot-native-openal-3d only supports the 'windows', 'linux' and 'macos' platforms."
    )

# Every platform + arch gets its own folder, named like the vaudio SDK package's folders (windows, windows-arm64, linux, linux-arm64, mac), since the x64 and arm64 libraries share file names
if platform == "macos":
    folder = "mac"
elif arch == "x86_64":
    folder = platform
elif arch == "arm64":
    folder = platform + "-arm64"
else:
    raise SCons.Errors.UserError("vaudio-godot-native-openal-3d only supports x86_64 and arm64 on " + platform + ", not " + arch + ".")

vaudio_libdir = "thirdparty/vaudio/lib/" + folder + "/"
openal_libdir = "thirdparty/openal/lib/" + folder + "/"
bin_dir = "addons/vaudio-godot-native-openal-3d/bin/" + folder + "/"

env.Append(LIBPATH=[vaudio_libdir])
env.Append(LIBS=["vaudionative"])

# Delay-load vaudionative.dll so a missing SDK doesn't stop the extension loading - common/va_sdk_check.cpp loads it at init and logs a readable error instead
if platform == "windows":
    env.Append(LINKFLAGS=["/DELAYLOAD:vaudionative.dll"])
    env.Append(LIBS=["delayimp"])

# godot-cpp already adds an $ORIGIN rpath on Linux; add the @loader_path equivalent on macOS so the libvaudionative / libopenal copies SConstruct drops beside the plugin are found at load time. build-unix.sh additionally rewrites libvaudionative's recorded path to @loader_path (it's linked with no -install_name).
if platform == "macos":
    env.Append(LINKFLAGS=["-Wl,-rpath,@loader_path"])

library = env.SharedLibrary(
    bin_dir + "libvaudiogodotnativeopenal3d{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)

def copy_beside_library(src_dir, name):
    env.Depends(library, env.Command(bin_dir + name, src_dir + name, Copy("$TARGET", "$SOURCE")))

# vaudionative and OpenAL Soft are loaded from beside the plugin binary (delay-load / $ORIGIN / @loader_path for vaudionative, LoadLibrary / dlopen for OpenAL - see src/openal/al_manager.cpp)
copy_beside_library(vaudio_libdir, vaudio_runtime_lib)
copy_beside_library(openal_libdir, openal_runtime_lib)

# The debug window only ships in vaudio's dev package, so copy it (and GLFW) only if vendored
for name in debug_window_files:
    if os.path.isfile(vaudio_libdir + name):
        copy_beside_library(vaudio_libdir, name)

env.NoCache(library)
Default(library)
