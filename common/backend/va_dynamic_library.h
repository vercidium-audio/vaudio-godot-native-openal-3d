// AI-created

#pragma once

// Audio backend libraries are loaded at runtime (LoadLibrary/GetProcAddress on Windows, dlopen/dlsym elsewhere) rather than linked, so the plugin loads without them
#ifdef _WIN32
#include <windows.h>
using va_dynamic_library = HMODULE;
#else
using va_dynamic_library = void *;
#endif

#include <string>

// Paths and names are UTF-8. nullptr if it failed to load
va_dynamic_library va_open_library(const char *path);

// An already loaded library with this file name, or nullptr. Doesn't load it
va_dynamic_library va_find_loaded_library(const char *file_name);

void *va_library_symbol(va_dynamic_library library, const char *name);

void va_close_library(va_dynamic_library library);

// The folder holding this GDExtension binary, ending in a slash, or empty if it can't be found
std::string va_get_own_module_directory();
