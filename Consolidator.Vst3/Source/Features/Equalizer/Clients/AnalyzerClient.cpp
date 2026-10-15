#include "Features/Equalizer/Clients/AnalyzerClient.h"

#include <array>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace consolidator::vst3::features::equalizer::clients
{

AnalyzerClient::AnalyzerClient(shared::clients::ProtocolClient& commands,
    const shared::clients::TargetStateClient& state)
    : commands_(commands), state_(state)
{
}

void AnalyzerClient::Invalidate()
{
    banksRequest_ = 0;
    banksDirty_ = false;
}

void AnalyzerClient::FetchBanks()
{
    if (banksRequest_ != 0)
    {
        banksDirty_ = true;
        return;
    }
    banksDirty_ = false;
    banksRequest_ = commands_.Request("observe_banks",
        { std::to_string(state_.Target().instance), std::string("equalizer") });
}

std::optional<features::equalizer::clients::BankSnapshots> AnalyzerClient::HandleBanks(const shared::protocol::ProtocolFrame& frame)
{
    const auto& values = frame.values;
    if (banksRequest_ == 0 || values.size() < 6 || shared::protocol::WireId(values[2]) != banksRequest_)
    {
        return {};
    }
    if (shared::protocol::WireId(values[3]) != state_.Target().instance ||
        shared::protocol::Symbol(values[4]) != "equalizer" || shared::protocol::Integer(values[5]) != 7)
    {
        throw std::invalid_argument("Invalid analyzer bank snapshots header.");
    }
    features::equalizer::clients::BankSnapshots banks;
    std::array<bool, 7> seen{};
    std::size_t offset = 6;
    for (int index = 0; index < 7; ++index)
    {
        if (values.size() - offset < 2)
        {
            throw std::invalid_argument("Truncated analyzer bank snapshots.");
        }
        const auto bank = shared::protocol::Integer(values[offset++]);
        const auto count = shared::protocol::Integer(values[offset++]);
        if (bank < 0 || bank >= 7 || seen[static_cast<std::size_t>(bank)] || count <= 0 ||
            static_cast<std::uint64_t>(count) > (values.size() - offset) / 2)
        {
            throw std::invalid_argument("Invalid analyzer bank snapshots entries.");
        }
        seen[static_cast<std::size_t>(bank)] = true;
        auto& entries = banks[static_cast<std::size_t>(bank)];
        for (std::int64_t entry = 0; entry < count; ++entry)
        {
            const auto path = shared::protocol::Symbol(values[offset++]);
            entries.push_back({ path, values[offset++], {}, {}, {}, {} });
        }
    }
    if (offset != values.size())
    {
        throw std::invalid_argument("Extra analyzer bank snapshots values.");
    }
    banksRequest_ = 0;
    if (banksDirty_)
    {
        FetchBanks();
    }
    return banks;
}

std::optional<features::equalizer::clients::AnalyzerSpectrum> AnalyzerClient::HandleSpectrum(const shared::protocol::ProtocolFrame& frame) const
{
    if (!state_.Ready())
    {
        return {};
    }
    const auto& values = frame.values;
    if (values.size() < 5 || shared::protocol::Integer(values[0]) != 2)
    {
        throw std::invalid_argument("Invalid FFT frame header.");
    }
    if (static_cast<std::uint64_t>(shared::protocol::Integer(values[1])) != state_.Target().instance)
    {
        return {};
    }
    const auto size = shared::protocol::Integer(values[2]);
    const bool hasGroup = shared::protocol::Boolean(values[3]);
    const bool hasReference = shared::protocol::Boolean(values[4]);
    const std::size_t spectrumCount = 1 + static_cast<int>(hasGroup) + static_cast<int>(hasReference);
    if (size != 1024 || values.size() != 5 + 513 * spectrumCount)
    {
        throw std::invalid_argument("Invalid FFT frame shape.");
    }
    features::equalizer::clients::AnalyzerSpectrum spectrum{ static_cast<std::size_t>(size), {}, {}, {} };
    spectrum.source.reserve(513);
    spectrum.group.reserve(hasGroup ? 513 : 0);
    spectrum.reference.reserve(hasReference ? 513 : 0);
    const std::size_t sourceEnd = 5 + 513;
    const std::size_t groupEnd = sourceEnd + (hasGroup ? 513 : 0);
    for (std::size_t index = 5; index < values.size(); ++index)
    {
        const double value = shared::protocol::Number(values[index]);
        if (!std::isfinite(value))
        {
            throw std::invalid_argument("Non-finite FFT value.");
        }
        if (index < sourceEnd)
        {
            spectrum.source.push_back(value);
        }
        else if (index < groupEnd)
        {
            spectrum.group.push_back(value);
        }
        else
        {
            spectrum.reference.push_back(value);
        }
    }
    return spectrum;
}

} // namespace consolidator::vst3::features::equalizer::clients
