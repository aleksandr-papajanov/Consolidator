#pragma once

#include <cstdint>
#include <string>

namespace consolidator::vst3::features::tracks::bindings
{

enum class TrackIntentKind
{
    SelectBank,
    SelectProcessor,
    SetMute,
    SetSolo,
    SetInstanceBypass,
    SetBankBypass,
    SetProcessorBypass,
    ResetInstance,
    ResetBank,
    ResetProcessor,
    SelectionChanged
};

struct TrackIntent
{
    TrackIntentKind kind;
    std::uint64_t instance{};
    int bank{};
    std::string processor;
    bool value{};
    bool additive{};
};

} // namespace consolidator::vst3::features::tracks::bindings
