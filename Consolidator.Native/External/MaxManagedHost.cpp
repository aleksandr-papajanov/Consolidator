#include "MaxManagedHost.h"

#include <cstring>

#include <Windows.h>

#include "c74_min_api.h"

namespace consolidator::max
{

std::wstring ResolveManagedLibraryPath()
{
    char filename[c74::max::MAX_FILENAME_CHARS]{};
    std::strncpy(filename, "Consolidator.Managed.dll", sizeof(filename) - 1);
    short path = 0;
    c74::max::t_fourcc type = 0;
    if (c74::max::locatefile_extended(filename, &path, &type, nullptr, 0) != 0)
    {
        return {};
    }

    char absolutePath[c74::max::MAX_PATH_CHARS]{};
    if (c74::max::path_toabsolutesystempath(path, filename, absolutePath) != c74::max::MAX_ERR_NONE)
    {
        return {};
    }

    const auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, absolutePath, -1, nullptr, 0);
    if (count <= 0)
    {
        return {};
    }
    std::wstring result(static_cast<std::size_t>(count), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, absolutePath, -1, result.data(), count) == 0)
    {
        return {};
    }
    result.pop_back();
    return result;
}

void __cdecl WriteManagedLog(void*, const char* message) noexcept
{
    if (message != nullptr)
    {
        c74::max::post("[Consolidator.Managed] %s", message);
    }
}

} // namespace consolidator::max
