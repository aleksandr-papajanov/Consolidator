#include "Host/Plugin/WindowsPluginHost.h"

#include <vector>

#include <Windows.h>
#include <JuceHeader.h>

namespace consolidator::vst3::host::plugin
{

namespace
{
const int moduleAnchor = 0;
}

std::wstring ResolveManagedLibraryPath()
{
    HMODULE module = nullptr;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&moduleAnchor), &module) == 0)
    {
        WriteManagedLog(nullptr, "Unable to resolve the VST3 module.");
        return {};
    }

    std::vector<wchar_t> path(32768);
    const auto length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0 || length >= path.size())
    {
        WriteManagedLog(nullptr, "Unable to resolve the VST3 module path.");
        return {};
    }
    std::wstring result(path.data(), length);
    const auto separator = result.find_last_of(L"\\/");
    if (separator == std::wstring::npos)
    {
        WriteManagedLog(nullptr, "The VST3 module path is not absolute.");
        return {};
    }
    result.resize(separator + 1);
    return result + L"Consolidator.Managed.dll";
}

bool IsEditorWindowFocused(void* nativeHandle) noexcept
{
    const auto window = static_cast<HWND>(nativeHandle);
    return window != nullptr && GetAncestor(window, GA_ROOT) == GetForegroundWindow();
}

void __cdecl WriteManagedLog(void*, const char* message) noexcept
{
    if (message == nullptr)
    {
        return;
    }
    try
    {
        juce::Logger::writeToLog("[Consolidator.Managed] " + juce::String::fromUTF8(message));
    }
    catch (...)
    {
        OutputDebugStringA(message);
    }
}

} // namespace consolidator::vst3::host::plugin
