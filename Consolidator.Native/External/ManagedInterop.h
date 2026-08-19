#pragma once

#include <cstddef>
#include <cstdint>

#include "SharedDspState.h"

namespace consolidator::max
{

enum class NativeAtomType : std::uint8_t
{
    Integer = 1,
    Float = 2,
    Symbol = 3
};

struct NativeAtom
{
    NativeAtomType type;
    std::uint8_t reserved[7]{};

    union
    {
        std::int64_t integer;
        double floating;
        const char* symbol;
    };
};

using InstanceId = std::uint64_t;

using ManagedLogCallback = void (__cdecl *)(void*, const char*);

using ManagedOutputCallback = void (__cdecl *)(
    void*,
    const char*,
    const NativeAtom*,
    std::size_t);

} // namespace consolidator::max