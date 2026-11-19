# Overview

This repo is a native Godot GDExtension (C++) plugin wrapping the vaudio C SDK, with OpenAL
Soft as the audio backend, for non-Mono ("Native Godot") projects.

This plugin links against the packaged vaudionative shared library (vaudionative.dll / libvaudionative.so / libvaudionative.dylib), not the source. Windows, Linux and macOS are supported.

# How to use

To build on Windows, run build.bat (builds x86_64 and arm64, into `bin/windows` and `bin/windows-arm64`). To build on Linux/macOS, run build-unix.sh (auto-detects the host: Linux builds an x86_64 or arm64 .so into `bin/linux` or `bin/linux-arm64`, macOS builds a universal .dylib).

The vendored binaries under `thirdparty/` are per-platform and git-ignored - you supply them yourself. `{folder}` is the vaudio SDK package's platform folder name (`windows`, `windows-arm64`, `linux`, `linux-arm64`, `mac`), which SConstruct picks from `platform` + `arch`:
- `thirdparty/vaudio/lib/{folder}/` : `vaudionative.dll` + `vaudionative.lib` / `libvaudionative.so` / `libvaudionative.dylib` (+ `vaudio-debug-window[.exe]` and `glfw3.dll` / `libglfw.so.3` / `libglfw.3.dylib`, optional, dev builds only - see below)
- `thirdparty/openal/lib/{folder}/` : `soft_oal.dll` / `libopenal.so.1` / `libopenal.1.dylib`

Every platform + arch builds into its own `addons/vaudio-godot-native-openal-3d/bin/{folder}/`, since the x64 and arm64 libraries share file names. The `.gdextension`'s `[libraries]` point into these folders, and its `[dependencies]` add `vaudionative` + OpenAL to exports (a missing dependency file fails the export, so the optional dev debug window + GLFW are added by `VAHRTFExportPlugin::_export_file` instead, only when present). `plugin.gd` checks `bin/{folder}/` for vaudionative, using `OS.get_name()` + `Engine.get_architecture_name()`.

OpenAL Soft is loaded at runtime from beside the plugin binary (LoadLibrary on Windows, dlopen on Linux/macOS), so the matching OpenAL shared library is copied into `bin/{folder}/` by the build and ships in the release zip. There is no OpenAL import lib - all entry points are resolved manually in `ALManager` (`src/openal/al_manager.cpp`).

macOS builds are universal (arm64 + x86_64, minimum macOS 11.0), so the plugin runs on Apple Silicon and Intel Macs. The vendored `libopenal.1.dylib` is OpenAL Soft 1.25.2 built universal from source with `cmake -S openal-soft -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew -DALSOFT_EXAMPLES=OFF -DALSOFT_UTILS=OFF -DALSOFT_NO_CONFIG_UTIL=ON -DALSOFT_INSTALL=OFF -DALSOFT_REQUIRE_COREAUDIO=ON -DHAVE_WFUNCTION_EFFECTS=OFF`. `HAVE_WFUNCTION_EFFECTS=OFF` is needed with Xcode 27's SDK, which marks the CoreAudio render callback `nonblocking` and trips OpenAL's `-Werror=function-effects`; ignoring `/opt/homebrew` keeps arm64-only Homebrew libs out of the link.

The ARM64 OpenAL Soft 1.25.2 builds come from the `Vercidium/openal-soft-github-actions` fork at `f6e6df251` (the same commit as the x64 `soft_oal.dll` / `libopenal.so.1`), with its CI's `Win64-Release` / `Linux-Release` flags plus `-DALSOFT_UTILS=ON` for `openal-info`:
- `windows-arm64/soft_oal.dll` (sha256 `bfa6f429c00a2ecf003da7f1d2532eb5f90c66aa32c288da95a4d5f3229e02d1`): built on x64 Windows with VS 18 (MSVC 14.51), dynamic CRT like x64, so users need the ARM64 VC++ redist. `cmake -G "Visual Studio 18 2026" -A ARM64 -B build-windows-arm64 -DCMAKE_BUILD_TYPE=Release -DALSOFT_TESTS=ON -DALSOFT_BUILD_ROUTER=ON -DALSOFT_REQUIRE_WINMM=ON -DALSOFT_REQUIRE_DSOUND=ON -DALSOFT_REQUIRE_WASAPI=ON -DALSOFT_UTILS=ON .` then `cmake --build build-windows-arm64 --config Release` (the gtest discovery step fails since the ARM64 test exe can't run on x64, harmless).
- `linux-arm64/libopenal.so.1` (sha256 `21282eb6637941c532ef9b7525f105d9a672b8ad9cc4c64f951b98ceeccdac8d`, the real `libopenal.so.1.25.2` renamed): built on the Ubuntu 26.04 arm64 VM (gcc 15.2), needs GLIBC_2.43, same as the linux-arm64 `libvaudionative.so`. Needs `libpulse-dev portaudio19-dev libasound2-dev libjack-dev libpipewire-0.3-dev libdbus-1-dev`. `cmake -B build-linux-arm64 -DCMAKE_BUILD_TYPE=Release -DALSOFT_REQUIRE_RTKIT=ON -DALSOFT_REQUIRE_ALSA=ON -DALSOFT_REQUIRE_OSS=ON -DALSOFT_REQUIRE_PORTAUDIO=ON -DALSOFT_REQUIRE_PULSEAUDIO=ON -DALSOFT_REQUIRE_JACK=ON -DALSOFT_REQUIRE_PIPEWIRE=ON -DALSOFT_TESTS=ON -DALSOFT_UTILS=ON .` then `cmake --build build-linux-arm64 -j4`.

`vaudio-debug-window` renders the raytracing simulation to a separate window. It only ships in vaudio's `dev` package, not `production`, so the build scripts only copy it (and GLFW) into `bin/{folder}/` if it exists.

# Shared common folder

`common/` is shared with the 2D plugin: `../vaudio-godot-native-openal-2d/common` is a local Windows junction to this folder, so this repo is the source of truth. Keep everything in `common/` dimension-agnostic - anything 2D or 3D specific belongs in each repo's `src/`. Git follows the junction, so every edit here also shows as modified in the 2D repo, and both repos need committing together.

SConstruct builds object files into the gitignored `obj/` folder (`VariantDir`) rather than beside the sources, since both plugins would otherwise write the same object file names into the shared `common/` folder.

# C++ code style

- Allman brace style: opening curly braces go on their own new line (for functions, classes,
  control flow, everything), not K&R/"same line" style.
- 4 spaces per indent level, not tabs.
- Don't split comments across lines, keep it all on one line

Don't use braces if it'll just be one line inside the braces, i.e:

```
// Don't do this
if (arrays.is_empty())
{
    continue;
}

// Do this
if (arrays.is_empty())
    continue;

// Except if it's an assignment
if (arrays.is_empty())
{
    arrays = xyz;
}
```

# Logging

When logging info / warning / errors, use the NAMED functions in src/va_engine_util.h:

```c
#define VA_LOG_NAMED(...) (godot::UtilityFunctions::print(VA_LOG_TAG, get_name(), ": ", __VA_ARGS__))
#define VA_WARN_NAMED(...) (godot::UtilityFunctions::push_warning(VA_LOG_TAG, get_name(), ": ", __VA_ARGS__))
#define VA_ERROR_NAMED(...) (godot::UtilityFunctions::push_error(VA_LOG_TAG, get_name(), ": ", __VA_ARGS__))
```

If the code is not inside a Node class, use the non-NAMED versions:

```c
#define VA_LOG(...) (godot::UtilityFunctions::print(VA_LOG_TAG, __VA_ARGS__))
#define VA_WARN(...) (godot::UtilityFunctions::push_warning(VA_LOG_TAG, __VA_ARGS__))
#define VA_ERROR(...) (godot::UtilityFunctions::push_error(VA_LOG_TAG, __VA_ARGS__))
```

# Continuous Cleanup

While working, if you notice unclean logs, fix them:
- Use the NAMED variants above if possible
- Don't log internal function names like `VAWorld::set_size` instead make the error nice and human readable, like `Failed to set size` instead of `VAWorld::set_size failed`. Remember the user isn't working with this code, they are working from the Godot editor
- Many logs repeat this text: `(VAResult=", VAResultToString(result), ")"`, maybe add a new macro for VA_ERROR_RESULT and VA_ERROR_NAMED_RESULT that also takes the VAResult as a parameter and automatically appends it in the log.

# References

vaudio.h lives at `thirdparty/vaudio/include/vaudio.h`

See user_context.md for more info (may not exist, this file is git-ignored)