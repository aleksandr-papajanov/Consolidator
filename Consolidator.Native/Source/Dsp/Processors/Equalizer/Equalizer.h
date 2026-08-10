#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "Dsp/Processors/Equalizer/Filters/Filter.h"
#include "Dsp/Processors/DspDevice.h"

namespace consolidator::dsp
{

struct EqualizerRuntimeState
{
    bool bypass = false;
    bool isNeutral = true;
};

class Equalizer final : public DspDevice
{
public:
    explicit Equalizer(BankId bankId) noexcept
        : DspDevice(DeviceId::Equalizer, detail::ElementKind::Device, detail::ToIndex(bankId))
        , bankId_(bankId)
    {
    }

    explicit Equalizer(detail::ElementKind filterElementKind) noexcept
        : DspDevice(DeviceId::Equalizer, detail::ElementKind::Device, 0)
        , filterElementKind_(filterElementKind)
    {
    }

    void Process(
        const double* input,
        double* output,
        std::size_t frameCount,
        std::size_t channelCount) override;

    void AddFilter(std::unique_ptr<Filter> filter);

    void Prepare(double sampleRate, std::size_t channelCount);
    void Reset() noexcept;

    [[nodiscard]] double ProcessSample(double input) noexcept;

    [[nodiscard]] const std::optional<BankId>& GetBankId() const noexcept
    {
        return bankId_;
    }

    [[nodiscard]] bool IsNeutral() const noexcept override
    {
        return runtimeState_.isNeutral;
    }

    [[nodiscard]] const EqualizerRuntimeState& GetRuntimeState() const noexcept
    {
        return runtimeState_;
    }

    [[nodiscard]] std::size_t GetFilterCount() const noexcept
    {
        return filters_.size();
    }

    [[nodiscard]] Filter* GetFilter(std::size_t index) noexcept;

    [[nodiscard]] const Filter* GetFilter(
        std::size_t index) const noexcept;

    bool ApplyParameter(
        const core::StatePath& route,
        const ParameterVariant& value,
        std::size_t depth) override;

    bool StageRuntimeUpdate(
        const core::StatePath& route,
        const ParameterVariant& value) override;

    void CommitRuntimeUpdates() override;

private:
    bool ApplyOwnParameter(
        const core::StatePath& route,
        const ParameterVariant& value) override;
    void RecalculateRuntime() override;

    [[nodiscard]] Filter* FindFilter(
        detail::ElementKind elementKind,
        std::uint8_t elementIndex) noexcept;

    [[nodiscard]] const Filter* FindFilter(
        detail::ElementKind elementKind,
        std::uint8_t elementIndex) const noexcept;

    std::optional<BankId> bankId_;
    detail::ElementKind filterElementKind_ = detail::ElementKind::EqFilter;
    EqualizerRuntimeState runtimeState_;
    std::vector<std::unique_ptr<Filter>> filters_;
};

} // namespace consolidator::dsp
