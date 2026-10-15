#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "Transport/ManagedInterop.h"

namespace consolidator::tests
{

bool RunAbiContractTests()
{
    using consolidator::max::dsp::DspSnapshot;
    using consolidator::max::dsp::DspTelemetryExchange;
    using consolidator::max::transport::NativeAtom;
    using consolidator::max::transport::NativeAtomType;
    using consolidator::max::dsp::DspStateExchange;

    static_assert(std::is_standard_layout_v<NativeAtom>);
    static_assert(std::is_standard_layout_v<DspSnapshot>);
    static_assert(std::is_standard_layout_v<DspTelemetryExchange>);
    static_assert(std::is_standard_layout_v<DspStateExchange>);

    auto succeeded = true;
    succeeded &= Expect(
        sizeof(NativeAtom) == 16,
        "NativeAtom size does not match the Managed ABI.");
    succeeded &= Expect(
        alignof(NativeAtom) == alignof(std::uint64_t),
        "NativeAtom alignment does not match the Managed ABI.");
    succeeded &= Expect(
        sizeof(DspSnapshot) == 1484,
        "DspSnapshot size does not match the Managed ABI.");
    succeeded &= Expect(
        sizeof(DspStateExchange) == 4480,
        "DspStateExchange size does not match the Managed ABI.");
    succeeded &= Expect(
        offsetof(DspStateExchange, publishedIndex) == 4452,
        "DspStateExchange published index offset is incorrect.");
    succeeded &= Expect(
        offsetof(DspStateExchange, consumerIndex) == 4456,
        "DspStateExchange consumer index offset is incorrect.");
    succeeded &= Expect(
        offsetof(DspStateExchange, telemetry) == 4460,
        "DspStateExchange telemetry offset is incorrect.");
    succeeded &= Expect(
        static_cast<std::uint8_t>(NativeAtomType::Integer) == 1
            && static_cast<std::uint8_t>(NativeAtomType::Float) == 2
            && static_cast<std::uint8_t>(NativeAtomType::Symbol) == 3,
        "NativeAtom type values do not match the Managed ABI.");
    return succeeded;
}

} // namespace consolidator::tests
