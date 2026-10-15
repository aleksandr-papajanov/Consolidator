#pragma once

#include <optional>

#include "Shared/Controls/Dial/DialRuntimeOverlayViewModel.h"

namespace consolidator::vst3::features::channelstrip::gainstaging::viewmodels
{

// Presentation overlay only: telemetry never overwrites the accepted Managed parameter.
class RuntimeDialViewModel final : public shared::controls::dial::DialRuntimeOverlayViewModel
{
public:
    void SetEnabled(bool enabled);
    void ApplyTelemetry(std::optional<double> value, std::optional<double> fast = {});
    void ClearTelemetry();
    [[nodiscard]] bool Enabled() const override;
    [[nodiscard]] std::optional<double> DisplayValue() const override;
    [[nodiscard]] std::optional<double> FastPosition() const override;

private:
    std::optional<double> value_;
    std::optional<double> fast_;
    bool enabled_{};
};

} // namespace consolidator::vst3::features::channelstrip::gainstaging::viewmodels
