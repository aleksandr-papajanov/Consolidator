#pragma once

#include <cstdint>

namespace consolidator::vst3::features::tracks::bindings
{

enum class TrackIntentKind
{
    SelectBank,
    SetMute,
    SetSolo,
    SetInstanceBypass,
    SetBankBypass,
    ResetInstance,
    ResetBank,
    SelectionChanged
};

struct TrackIntent
{
    TrackIntentKind kind;
    std::uint64_t instance{};
    int bank{};
    bool value{};
    bool additive{};
};

} // namespace consolidator::vst3::features::tracks::bindings
