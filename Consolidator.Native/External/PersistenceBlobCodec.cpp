#include "PersistenceBlobCodec.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace consolidator::max
{

std::optional<std::vector<double>> PersistenceBlobCodec::Encode(
    std::string_view payload)
{
    if (payload.empty() || payload.size() > MaximumPayloadBytes)
    {
        return std::nullopt;
    }

    const auto chunkCount =
        (payload.size() + BytesPerAtom - 1) / BytesPerAtom;
    std::vector<double> atoms;
    atoms.reserve(HeaderAtomCount + chunkCount);
    atoms.push_back(Magic);
    atoms.push_back(static_cast<double>(payload.size()));

    for (std::size_t offset = 0; offset < payload.size();
        offset += BytesPerAtom)
    {
        std::uint64_t chunk = 0;
        const auto byteCount = std::min(
            BytesPerAtom,
            payload.size() - offset);
        for (std::size_t byteIndex = 0; byteIndex < byteCount; ++byteIndex)
        {
            chunk |= static_cast<std::uint64_t>(
                static_cast<unsigned char>(payload[offset + byteIndex]))
                << (byteIndex * 8);
        }

        atoms.push_back(static_cast<double>(chunk));
    }

    return atoms;
}

std::optional<std::string> PersistenceBlobCodec::Decode(
    std::span<const double> atoms)
{
    if (atoms.size() < HeaderAtomCount ||
        atoms.size() > MaximumAttributeAtoms)
    {
        return std::nullopt;
    }

    for (const auto atom : atoms)
    {
        if (!std::isfinite(atom) || std::trunc(atom) != atom ||
            atom < 0 || atom > MaximumPackedAtom)
        {
            return std::nullopt;
        }
    }

    if (atoms[0] != Magic || atoms[1] <= 0 ||
        atoms[1] > static_cast<double>(MaximumPayloadBytes))
    {
        return std::nullopt;
    }

    const auto payloadSize = static_cast<std::size_t>(atoms[1]);
    const auto chunkCount =
        (payloadSize + BytesPerAtom - 1) / BytesPerAtom;
    if (atoms.size() != HeaderAtomCount + chunkCount)
    {
        return std::nullopt;
    }

    std::string payload(payloadSize, '\0');
    for (std::size_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
    {
        const auto packed = static_cast<std::uint64_t>(
            atoms[HeaderAtomCount + chunkIndex]);
        const auto offset = chunkIndex * BytesPerAtom;
        const auto byteCount = std::min(
            BytesPerAtom,
            payloadSize - offset);
        if (byteCount < BytesPerAtom &&
            (packed >> (byteCount * 8)) != 0)
        {
            return std::nullopt;
        }
        for (std::size_t byteIndex = 0; byteIndex < byteCount; ++byteIndex)
        {
            payload[offset + byteIndex] = static_cast<char>(
                (packed >> (byteIndex * 8)) & 0xFF);
        }
    }

    return payload;
}

} // namespace consolidator::max
