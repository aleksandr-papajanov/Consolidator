#pragma once

#include <compare>
#include <cstdint>
#include <functional>
#include <variant>

namespace consolidator::dsp
{

// Strongly typed public identifiers.

enum class DeviceId : std::uint8_t
{
    MainInputGain,
    MainOutputGain,
    Saturator,
    Compressor,
    Equalizer
};

enum class EqFilterId : std::uint8_t
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

constexpr std::uint8_t ToIndex(EqFilterId id) noexcept
{
    return static_cast<std::uint8_t>(id);
}

constexpr std::uint8_t ToIndex(SaturatorDetectorFilterId id) noexcept
{
    return static_cast<std::uint8_t>(id);
}

constexpr std::uint8_t ToIndex(CompressorDetectorFilterId id) noexcept
{
    return static_cast<std::uint8_t>(id);
}

constexpr EqFilterId ToEqFilterId(std::uint8_t index) noexcept
{
    return static_cast<EqFilterId>(index);
}

constexpr SaturatorDetectorFilterId ToSatDetectorId(std::uint8_t index) noexcept
{
    return static_cast<SaturatorDetectorFilterId>(index);
}

constexpr CompressorDetectorFilterId ToCompDetectorId(std::uint8_t index) noexcept
{
    return static_cast<CompressorDetectorFilterId>(index);
}

} // namespace detail

class ParameterAddress final
{
public:
    // Gain devices.

    static constexpr ParameterAddress MainInputGain() noexcept
    {
        return Device(DeviceId::MainInputGain, ParameterId::Gain);
    }

    static constexpr ParameterAddress MainInputBypass() noexcept
    {
        return Device(DeviceId::MainInputGain, ParameterId::Bypass);
    }

    static constexpr ParameterAddress MainOutputGain() noexcept
    {
        return Device(DeviceId::MainOutputGain, ParameterId::Gain);
    }

    static constexpr ParameterAddress MainOutputBypass() noexcept
    {
        return Device(DeviceId::MainOutputGain, ParameterId::Bypass);
    }

    // Saturator.

    static constexpr ParameterAddress SaturatorDrive() noexcept
    {
        return Device(DeviceId::Saturator, ParameterId::Drive);
    }

    static constexpr ParameterAddress SaturatorOutputGain() noexcept
    {
        return Device(DeviceId::Saturator, ParameterId::OutputGain);
    }

    static constexpr ParameterAddress SaturatorMix() noexcept
    {
        return Device(DeviceId::Saturator, ParameterId::Mix);
    }

    static constexpr ParameterAddress SaturatorBypass() noexcept
    {
        return Device(DeviceId::Saturator, ParameterId::Bypass);
    }

    static constexpr ParameterAddress SaturatorDetectorFrequency(
        SaturatorDetectorFilterId filter) noexcept
    {
        return SaturatorDetector(filter, ParameterId::Frequency);
    }

    static constexpr ParameterAddress SaturatorDetectorQ(
        SaturatorDetectorFilterId filter) noexcept
    {
        return SaturatorDetector(filter, ParameterId::Q);
    }

    static constexpr ParameterAddress SaturatorDetectorGain(
        SaturatorDetectorFilterId filter) noexcept
    {
        return SaturatorDetector(filter, ParameterId::Gain);
    }

    static constexpr ParameterAddress SaturatorDetectorBypass(
        SaturatorDetectorFilterId filter) noexcept
    {
        return SaturatorDetector(filter, ParameterId::Bypass);
    }

    // Compressor.

    static constexpr ParameterAddress CompressorThreshold() noexcept
    {
        return Device(DeviceId::Compressor, ParameterId::Threshold);
    }

    static constexpr ParameterAddress CompressorRatio() noexcept
    {
        return Device(DeviceId::Compressor, ParameterId::Ratio);
    }

    static constexpr ParameterAddress CompressorAttack() noexcept
    {
        return Device(DeviceId::Compressor, ParameterId::Attack);
    }

    static constexpr ParameterAddress CompressorRelease() noexcept
    {
        return Device(DeviceId::Compressor, ParameterId::Release);
    }

    static constexpr ParameterAddress CompressorOutputGain() noexcept
    {
        return Device(DeviceId::Compressor, ParameterId::OutputGain);
    }

    static constexpr ParameterAddress CompressorMix() noexcept
    {
        return Device(DeviceId::Compressor, ParameterId::Mix);
    }

    static constexpr ParameterAddress CompressorBypass() noexcept
    {
        return Device(DeviceId::Compressor, ParameterId::Bypass);
    }

    static constexpr ParameterAddress CompressorDetectorFrequency(
        CompressorDetectorFilterId filter) noexcept
    {
        return CompressorDetector(filter, ParameterId::Frequency);
    }

    static constexpr ParameterAddress CompressorDetectorQ(
        CompressorDetectorFilterId filter) noexcept
    {
        return CompressorDetector(filter, ParameterId::Q);
    }

    static constexpr ParameterAddress CompressorDetectorGain(
        CompressorDetectorFilterId filter) noexcept
    {
        return CompressorDetector(filter, ParameterId::Gain);
    }

    static constexpr ParameterAddress CompressorDetectorBypass(
        CompressorDetectorFilterId filter) noexcept
    {
        return CompressorDetector(filter, ParameterId::Bypass);
    }

    // Equalizer.

    static constexpr ParameterAddress EqFilterFrequency(EqFilterId filter) noexcept
    {
        return EqFilter(filter, ParameterId::Frequency);
    }

    static constexpr ParameterAddress EqFilterQ(EqFilterId filter) noexcept
    {
        return EqFilter(filter, ParameterId::Q);
    }

    static constexpr ParameterAddress EqFilterGain(EqFilterId filter) noexcept
    {
        return EqFilter(filter, ParameterId::Gain);
    }

    static constexpr ParameterAddress EqFilterBypass(EqFilterId filter) noexcept
    {
        return EqFilter(filter, ParameterId::Bypass);
    }

    static constexpr ParameterAddress EqFilterType(EqFilterId filter) noexcept
    {
        return EqFilter(filter, ParameterId::Type);
    }

    static constexpr ParameterAddress EqChainBypass() noexcept
    {
        return Device(DeviceId::Equalizer, ParameterId::Bypass);
    }

    static constexpr ParameterAddress EqChainSolo() noexcept
    {
        return Device(DeviceId::Equalizer, ParameterId::Solo);
    }

    // Accessors.

    [[nodiscard]] constexpr DeviceId GetDeviceId() const noexcept
    {
        return deviceId_;
    }

    [[nodiscard]] constexpr ParameterId GetParameterId() const noexcept
    {
        return parameterId_;
    }

    [[nodiscard]] constexpr detail::ElementKind GetElementKind() const noexcept
    {
        return elementKind_;
    }

    [[nodiscard]] constexpr std::uint8_t GetElementIndex() const noexcept
    {
        return elementIndex_;
    }

    [[nodiscard]] constexpr bool IsDeviceParameter() const noexcept
    {
        return elementKind_ == detail::ElementKind::Device;
    }

    [[nodiscard]] constexpr bool IsEqFilterParameter() const noexcept
    {
        return elementKind_ == detail::ElementKind::EqFilter;
    }

    [[nodiscard]] constexpr bool IsSaturatorDetectorParameter() const noexcept
    {
        return elementKind_ == detail::ElementKind::SaturatorDetectorFilter;
    }

    [[nodiscard]] constexpr bool IsCompressorDetectorParameter() const noexcept
    {
        return elementKind_ == detail::ElementKind::CompressorDetectorFilter;
    }

    [[nodiscard]] constexpr EqFilterId GetEqFilterId() const noexcept
    {
        return detail::ToEqFilterId(elementIndex_);
    }

    [[nodiscard]] constexpr SaturatorDetectorFilterId GetSatDetectorFilterId() const noexcept
    {
        return detail::ToSatDetectorId(elementIndex_);
    }

    [[nodiscard]] constexpr CompressorDetectorFilterId GetCompDetectorFilterId() const noexcept
    {
        return detail::ToCompDetectorId(elementIndex_);
    }

    [[nodiscard]] constexpr std::uint32_t ToKey() const noexcept
    {
        return static_cast<std::uint32_t>(deviceId_)
            | (static_cast<std::uint32_t>(elementKind_) << 8U)
            | (static_cast<std::uint32_t>(elementIndex_) << 16U)
            | (static_cast<std::uint32_t>(parameterId_) << 24U);
    }

    friend constexpr auto operator<=>(
        const ParameterAddress&,
        const ParameterAddress&) noexcept = default;

private:
    constexpr ParameterAddress(
        DeviceId deviceId,
        detail::ElementKind elementKind,
        std::uint8_t elementIndex,
        ParameterId parameterId) noexcept
        : deviceId_(deviceId)
        , elementKind_(elementKind)
        , elementIndex_(elementIndex)
        , parameterId_(parameterId)
    {
    }

    static constexpr ParameterAddress Device(DeviceId deviceId, ParameterId parameterId) noexcept
    {
        return {deviceId, detail::ElementKind::Device, 0, parameterId};
    }

    static constexpr ParameterAddress EqFilter(EqFilterId filterId, ParameterId parameterId) noexcept
    {
        return {
            DeviceId::Equalizer,
            detail::ElementKind::EqFilter,
            detail::ToIndex(filterId),
            parameterId};
    }

    static constexpr ParameterAddress SaturatorDetector(
        SaturatorDetectorFilterId filterId,
        ParameterId parameterId) noexcept
    {
        return {
            DeviceId::Saturator,
            detail::ElementKind::SaturatorDetectorFilter,
            detail::ToIndex(filterId),
            parameterId};
    }

    static constexpr ParameterAddress CompressorDetector(
        CompressorDetectorFilterId filterId,
        ParameterId parameterId) noexcept
    {
        return {
            DeviceId::Compressor,
            detail::ElementKind::CompressorDetectorFilter,
            detail::ToIndex(filterId),
            parameterId};
    }

    DeviceId deviceId_;
    detail::ElementKind elementKind_;
    std::uint8_t elementIndex_;
    ParameterId parameterId_;
};

} // namespace consolidator::dsp

template <>
struct std::hash<consolidator::dsp::ParameterAddress>
{
    constexpr std::size_t operator()(
        const consolidator::dsp::ParameterAddress& address) const noexcept
    {
        return static_cast<std::size_t>(address.ToKey());
    }
};