#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "ManagedInterop.h"

namespace consolidator::tests
{

bool RunAbiContractTests()
{
    using consolidator::max::DspSnapshot;
    using consolidator::max::NativeAtom;
    using consolidator::max::NativeAtomType;
    using consolidator::max::DspStateExchange;

    static_assert(std::is_standard_layout_v<NativeAtom>);
    static_assert(std::is_standard_layout_v<DspSnapshot>);
    static_assert(std::is_standard_layout_v<DspStateExchange>);

    auto succeeded = true;
    succeeded &= Expect(
        sizeof(NativeAtom) == 16,
        "NativeAtom size does not match the Managed ABI.");
    succeeded &= Expect(
        alignof(NativeAtom) == alignof(std::uint64_t),
        "NativeAtom alignment does not match the Managed ABI.");
    succeeded &= Expect(
        sizeof(DspSnapshot) == 372,
        "DspSnapshot size does not match the Managed ABI.");
    succeeded &= Expect(
        sizeof(DspStateExchange) == 4424,
        "DspStateExchange size does not match the Managed ABI.");
    succeeded &= Expect(
        offsetof(DspStateExchange, publishedIndex) == 4416,
        "DspStateExchange published index offset is incorrect.");
    succeeded &= Expect(
        offsetof(DspStateExchange, consumerIndex) == 4420,
        "DspStateExchange consumer index offset is incorrect.");
    succeeded &= Expect(
        static_cast<std::uint8_t>(NativeAtomType::Integer) == 1
            && static_cast<std::uint8_t>(NativeAtomType::Float) == 2
            && static_cast<std::uint8_t>(NativeAtomType::Symbol) == 3,
        "NativeAtom type values do not match the Managed ABI.");
    return succeeded;
}

} // namespace consolidator::tests
