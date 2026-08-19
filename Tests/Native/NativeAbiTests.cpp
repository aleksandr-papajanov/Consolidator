#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>

#include "ManagedInterop.h"

using consolidator::max::NativeAtom;
using consolidator::max::NativeAtomType;

int main()
{
    static_assert(std::is_standard_layout_v<NativeAtom>);
    static_assert(sizeof(NativeAtom) == 16);
    static_assert(alignof(NativeAtom) == alignof(std::uint64_t));

    if (static_cast<std::uint8_t>(NativeAtomType::Integer) != 1
        || static_cast<std::uint8_t>(NativeAtomType::Float) != 2
        || static_cast<std::uint8_t>(NativeAtomType::Symbol) != 3)
    {
        std::cerr << "NativeAtom type values do not match the ABI contract.\n";
        return 1;
    }

    return 0;
}
