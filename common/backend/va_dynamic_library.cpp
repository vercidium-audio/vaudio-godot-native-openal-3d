// AI-created

#include "va_dynamic_library.h"

#ifndef _WIN32
#include <dlfcn.h>
#endif

#ifdef _WIN32
static std::wstring utf8_to_wide(const char *text)
{
    int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);

    if (length <= 0)
        return std::wstring();

    std::wstring result((size_t)length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text, -1, result.data(), length);
    result.resize((size_t)length - 1);
    return result;
}
#endif

va_dynamic_library va_open_library(const char *path)
{
#ifdef _WIN32
    return LoadLibraryW(utf8_to_wide(path).c_str());
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

va_dynamic_library va_find_loaded_library(const char *file_name)
{
#ifdef _WIN32
    return GetModuleHandleW(utf8_to_wide(file_name).c_str());
#else
    // RTLD_NOLOAD only returns a handle when it's already loaded. It adds a reference, which is never released, since audio libraries stay loaded
    return dlopen(file_name, RTLD_NOW | RTLD_LOCAL | RTLD_NOLOAD);
#endif
}

void *va_library_symbol(va_dynamic_library library, const char *name)
{
#ifdef _WIN32
    return reinterpret_cast<void *>(GetProcAddress(library, name));
#else
    return dlsym(library, name);
#endif
}

void va_close_library(va_dynamic_library library)
{
#ifdef _WIN32
    FreeLibrary(library);
#else
    dlclose(library);
#endif
}

std::string va_get_own_module_directory()
{
#ifdef _WIN32
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&va_get_own_module_directory), &self);

    wchar_t path[MAX_PATH];
    DWORD length = GetModuleFileNameW(self, path, MAX_PATH);

    if (length == 0 || length == MAX_PATH)
        return "";

    // UTF-8, like every other path handed to va_open_library
    int utf8_length = WideCharToMultiByte(CP_UTF8, 0, path, (int)length, nullptr, 0, nullptr, nullptr);
    std::string result((size_t)utf8_length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, path, (int)length, result.data(), utf8_length, nullptr, nullptr);
#else
    Dl_info info;

    if (dladdr(reinterpret_cast<const void *>(&va_get_own_module_directory), &info) == 0 || !info.dli_fname)
        return "";

    std::string result(info.dli_fname);
#endif

    size_t last_slash = result.find_last_of("\\/");

    if (last_slash == std::string::npos)
        return "";

    return result.substr(0, last_slash + 1);
}
