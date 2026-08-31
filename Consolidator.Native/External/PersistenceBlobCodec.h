#pragma once

#include <cstddef>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace consolidator::max
{

class PersistenceBlobCodec
{
public:
    static constexpr std::size_t MaximumPayloadBytes = 380 * 1024;

    [[nodiscard]] static std::optional<std::vector<double>> Encode(
        std::string_view payload);

    [[nodiscard]] static std::optional<std::string> Decode(
        std::span<const double> atoms);

private:
    static constexpr double Magic = 0x43505331;
    static constexpr std::size_t HeaderAtomCount = 2;
    static constexpr std::size_t BytesPerAtom = 6;
    static constexpr std::size_t MaximumAttributeAtoms = 0xFFFF;
    static constexpr double MaximumPackedAtom = 281474976710655.0;
    static_assert(
        std::numeric_limits<double>::digits >= BytesPerAtom * 8);
    static_assert(
        HeaderAtomCount +
            (MaximumPayloadBytes + BytesPerAtom - 1) / BytesPerAtom <=
        MaximumAttributeAtoms);
};

} // namespace consolidator::max
