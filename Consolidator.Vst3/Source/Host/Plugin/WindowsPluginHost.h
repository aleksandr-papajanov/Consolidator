#pragma once

#include <string>

namespace consolidator::vst3::host::plugin
{

[[nodiscard]] std::wstring ResolveManagedLibraryPath();
[[nodiscard]] bool IsEditorWindowFocused(void* nativeHandle) noexcept;
void __cdecl WriteManagedLog(void* context, const char* message) noexcept;

} // namespace consolidator::vst3::host::plugin
