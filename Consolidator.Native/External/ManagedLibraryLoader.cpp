#if defined(_WIN32)

#include <cstring>

#include "c74_min_api.h"

#include <delayimp.h>

namespace
{

constexpr auto kCoreLibraryName = "ConsolidatorCore.dll";

FARPROC WINAPI LoadCoreLibrary(
    unsigned notification,
    PDelayLoadInfo delayInfo)
{
    if (notification != dliNotePreLoadLibrary || delayInfo == nullptr ||
        delayInfo->szDll == nullptr ||
        _stricmp(delayInfo->szDll, kCoreLibraryName) != 0)
    {
        return nullptr;
    }

    if (const auto loaded = GetModuleHandleA(kCoreLibraryName))
    {
        return reinterpret_cast<FARPROC>(loaded);
    }

    char filename[c74::max::MAX_FILENAME_CHARS]{};
    std::strncpy(filename, kCoreLibraryName, sizeof(filename) - 1);
    short path = 0;
    c74::max::t_fourcc type = 0;
    if (c74::max::locatefile_extended(filename, &path, &type, nullptr, 0) != 0)
    {
        return nullptr;
    }

    char absolutePath[c74::max::MAX_PATH_CHARS]{};
    if (c74::max::path_toabsolutesystempath(path, filename, absolutePath) !=
        c74::max::MAX_ERR_NONE)
    {
        return nullptr;
    }

    return reinterpret_cast<FARPROC>(LoadLibraryA(absolutePath));
}

} // namespace

extern "C"
{
const PfnDliHook __pfnDliNotifyHook2 = LoadCoreLibrary;
}

#endif
