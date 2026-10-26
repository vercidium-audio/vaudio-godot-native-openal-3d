#include "va_sdk_check.h"

#ifdef _WIN32
#include <windows.h>

#include <string>
#endif

#include "va_engine_util.h"

namespace va_godot
{

bool load_vaudio_sdk()
{
#ifdef _WIN32
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&load_vaudio_sdk), &self);

    wchar_t module_path[MAX_PATH];
    DWORD length = GetModuleFileNameW(self, module_path, MAX_PATH);

    // Can't resolve our own folder - let the delay-load helper search the default paths instead
    if (length == 0 || length == MAX_PATH)
        return true;

    std::wstring dir(module_path, length);
    dir = dir.substr(0, dir.find_last_of(L"\\/") + 1);

    // Once loaded by full path, the delay-load helper's LoadLibrary("vaudionative.dll") resolves to this same module
    std::wstring sdk_path = dir + L"vaudionative.dll";
    if (LoadLibraryExW(sdk_path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH))
        return true;

    DWORD load_error = GetLastError();

    // In the editor this runs before the Output panel exists, so the error would only reach stdout - plugin.gd logs it instead once the editor is up
    if (IS_EDITOR_HINT())
        return false;

    if (GetFileAttributesW(sdk_path.c_str()) == INVALID_FILE_ATTRIBUTES)
        VA_ERROR("vaudionative.dll is missing - the plugin is disabled. Copy vaudionative.dll from the Vercidium Audio SDK's native/production/windows folder (or native/dev/windows, along with vaudio-debug-window.exe and glfw3.dll for the debug window) into ", dir.c_str(), " then restart Godot.");
    else
        VA_ERROR("Failed to load ", sdk_path.c_str(), " (Windows error ", (int64_t)load_error, ") - the plugin is disabled. Make sure it's the 64-bit build from the same Vercidium Audio SDK version as this plugin, then restart Godot.");

    return false;
#else
    return true;
#endif
}

} // namespace va_godot
