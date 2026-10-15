#pragma once

#include <string>

#include "Transport/ManagedInterop.h"

namespace consolidator::max
{

[[nodiscard]] std::wstring ResolveManagedLibraryPath();
void __cdecl WriteManagedLog(void* context, const char* message) noexcept;

} // namespace consolidator::max
