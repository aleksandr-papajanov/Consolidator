#pragma once

#include <cstdint>

namespace consolidator::dsp
{

enum class RouteNodeId : std::uint8_t
{
    Detector,
    Bank0,
    Bank1,
    Bank2,
    Bank3,
    Bank4,
    Bank5,
    Bank6,
    Filter1,
    Filter2,
    Filter3,
    Filter4,
    Filter5,
    Filter6,
    Filter7
};

enum class DeviceId : std::uint8_t
{
    MainInputGain,
    MainOutputGain,
    Saturator,
    Compressor,
    Equalizer
};

enum class BankId : std::uint8_t
{
    Bank0,
    Bank1,
    Bank2,
    Bank3,
    Bank4,
    Bank5,
    Bank6
};

enum class FilterId : std::uint8_t
{
    Filter1,
    Filter2,
    Filter3,
    Filter4,
    Filter5,
    Filter6,
    Filter7
};

enum class SaturatorDetectorFilterId : std::uint8_t
{
    Filter1,
    Filter2
};

enum class CompressorDetectorFilterId : std::uint8_t
{
    Filter1,
    Filter2
};

enum class ParameterId : std::uint8_t
{
    Gain,
    Threshold,
    Ratio,
    Attack,
    Release,
    Drive,
    Frequency,
    Q,
    OutputGain,
    Bypass,
    Mix,
    Type,
    Solo
};

namespace detail
{

enum class ElementKind : std::uint8_t
{
    Device,
    EqFilter,
    SaturatorDetectorFilter,
    CompressorDetectorFilter
};

constexpr std::uint8_t ToIndex(BankId id) noexcept
{
    return static_cast<std::uint8_t>(id);
}

constexpr std::uint8_t ToIndex(FilterId id) noexcept
{
    return static_cast<std::uint8_t>(id);
}

constexpr std::uint8_t ToIndex(
    SaturatorDetectorFilterId id) noexcept
{
    return static_cast<std::uint8_t>(id);
}

constexpr std::uint8_t ToIndex(
    CompressorDetectorFilterId id) noexcept
{
    return static_cast<std::uint8_t>(id);
}

constexpr FilterId ToFilterId(std::uint8_t index) noexcept
{
    return static_cast<FilterId>(index);
}

} // namespace detail

} // namespace consolidator::dsp
