#!/usr/bin/env bash
# Linux/macOS counterpart of build.bat. Linux builds an x86_64 or arm64 .so (the host's arch), macOS builds a universal (arm64 + x86_64) .dylib.
set -euo pipefail
cd "$(dirname "$0")"

if [ -f .env ]; then
    set -a
    . ./.env
    set +a
fi

# Bootstrap extern/godot-cpp on a fresh clone; a no-op once initialised.
git submodule update --init --recursive

case "$(uname -s)" in
    Linux)
        PLATFORM=linux; LIBEXT=".so"; EXTRA_SCONS_ARGS=""
        case "$(uname -m)" in
            x86_64) ARCH=x86_64; FOLDER=linux ;;
            aarch64) ARCH=arm64; FOLDER=linux-arm64 ;;
            *) echo "Unsupported Linux architecture: $(uname -m)" >&2; exit 1 ;;
        esac
        ;;
    Darwin)
        PLATFORM=macos; ARCH=universal; FOLDER=mac; LIBEXT=".dylib"
        # Without a deployment target the plugin only loads on the build machine's macOS version or newer. 11.0 is the first arm64 macOS, and matches the vendored universal libopenal.1.dylib.
        EXTRA_SCONS_ARGS="macos_deployment_target=11.0"
        # Non-interactive SSH gets a bare PATH without Homebrew, where scons lives.
        for brew_bin in /opt/homebrew/bin /usr/local/bin; do
            [ -x "$brew_bin/scons" ] && case ":$PATH:" in *":$brew_bin:"*) ;; *) PATH="$brew_bin:$PATH" ;; esac
        done
        ;;
    *) echo "Unsupported host: $(uname -s)" >&2; exit 1 ;;
esac

# Invalid doc XML (e.g. a bare < in a description) silently breaks every editor tooltip
if command -v xmllint >/dev/null 2>&1; then
    xmllint --noout doc_classes/*.xml
else
    echo "WARNING: xmllint not found - skipping doc_classes XML validation"
fi

scons platform="$PLATFORM" arch="$ARCH" target=editor $EXTRA_SCONS_ARGS
scons platform="$PLATFORM" arch="$ARCH" target=template_debug $EXTRA_SCONS_ARGS
scons platform="$PLATFORM" arch="$ARCH" target=template_release $EXTRA_SCONS_ARGS

# SConstruct writes the plugin, vaudionative, OpenAL and (when vendored) the debug window + GLFW into bin/{FOLDER}
ADDON_DIR="addons/vaudio-godot-native-openal-3d"
BIN_DIR="$ADDON_DIR/bin/$FOLDER"

if [ "$PLATFORM" = "macos" ]; then
    # libvaudionative.dylib is linked with no -install_name, so it records its absolute build-machine path -
    # rewrite the plugin's reference to it (and its own id) to @loader_path so the copy shipped beside it resolves.
    for target in editor template_debug template_release; do
        plugin="$BIN_DIR/libvaudiogodotnativeopenal3d.${PLATFORM}.${target}.${ARCH}${LIBEXT}"
        dep=$(otool -L "$plugin" | awk '/libvaudionative\.dylib/ {print $1; exit}')
        [ -n "$dep" ] && install_name_tool -change "$dep" "@loader_path/libvaudionative.dylib" "$plugin"
    done
    install_name_tool -id "@loader_path/libvaudionative.dylib" "$BIN_DIR/libvaudionative.dylib" 2>/dev/null || true
fi

[ -f "$BIN_DIR/vaudio-debug-window" ] && chmod +x "$BIN_DIR/vaudio-debug-window"

copy_to() {
    local dest="$1/bin/$FOLDER"
    [ -d "$1" ] || { echo "$1 missing - skipping"; return; }
    echo "Copying build output to $dest"
    mkdir -p "$dest"
    cp -f "$BIN_DIR"/* "$dest/"
    cp -f "$ADDON_DIR/plugin.cfg" "$ADDON_DIR/plugin.gd" "$ADDON_DIR/vaudio_godot_native_openal_3d.gdextension" "$1/"
}

if [ -n "${VAUDIO_RELEASE_DIR:-}" ]; then copy_to "$VAUDIO_RELEASE_DIR"; else echo "VAUDIO_RELEASE_DIR not set - skipping copy to release repo"; fi
if [ -n "${VAUDIO_DEMO_ADDON_DIR:-}" ]; then copy_to "$VAUDIO_DEMO_ADDON_DIR"; else echo "VAUDIO_DEMO_ADDON_DIR not set - skipping copy to demo project addon"; fi
