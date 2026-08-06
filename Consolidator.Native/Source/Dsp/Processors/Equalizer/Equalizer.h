#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "Dsp/Parameters/ParameterAddress.h"
#include "Dsp/Processors/Equalizer/Filters/Filter.h"
#include "Dsp/Processors/IDspDevice.h"

namespace consolidator::dsp
{

class Equalizer final : public IDspDevice
{
public:
    explicit Equalizer(BankId bankId) noexcept
        : bankId_(bankId)
    {
    }

    void Process(
        const double* input,
        double* output,
        std::size_t frameCount,
        std::size_t channelCount) override;

    void ApplyParameterChange(
        const ParameterChange& change) override;

    void AddFilter(std::unique_ptr<Filter> filter);

    [[nodiscard]] DeviceId GetDeviceId() const noexcept override
    {
        return DeviceId::Equalizer;
    }

    [[nodiscard]] detail::ElementKind GetElementKind() const noexcept override
    {
        return detail::ElementKind::Device;
    }

    [[nodiscard]] std::uint8_t GetElementIndex() const noexcept override
    {
        return detail::ToIndex(bankId_);
    }

    [[nodiscard]] BankId GetBankId() const noexcept
    {
        return bankId_;
    }

    [[nodiscard]] bool IsNeutral() const noexcept override
    {
        for (const auto& f : filters_)
        {
            if (!f->IsNeutral())
            {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] std::size_t GetFilterCount() const noexcept
    {
        return filters_.size();
    }

    [[nodiscard]] Filter* GetFilter(std::size_t index) noexcept;

    [[nodiscard]] const Filter* GetFilter(
        std::size_t index) const noexcept;

private:
    [[nodiscard]] Filter* FindFilter(
        detail::ElementKind elementKind,
        std::uint8_t elementIndex) noexcept;

    [[nodiscard]] const Filter* FindFilter(
        detail::ElementKind elementKind,
        std::uint8_t elementIndex) const noexcept;

    BankId bankId_;
    std::vector<std::unique_ptr<Filter>> filters_;
};

} // namespace consolidator::dsp