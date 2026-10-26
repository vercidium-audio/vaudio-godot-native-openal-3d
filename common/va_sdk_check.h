#pragma once

namespace va_godot
{

// On Windows vaudionative.dll is delay-loaded (see SConstruct), so this loads it from beside our own module and logs a readable error if it's missing, instead of Godot's "Error 126" spam. Always true on Linux/macOS, where the SDK is linked normally.
bool load_vaudio_sdk();

} // namespace va_godot
