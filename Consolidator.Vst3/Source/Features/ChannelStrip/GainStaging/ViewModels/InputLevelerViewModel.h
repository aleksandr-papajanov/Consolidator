#pragma once

#include <optional>
#include <utility>

#include "Shared/Controls/Dial/DialRuntimeOverlayViewModel.h"

namespace consolidator::vst3::features::channelstrip::gainstaging::viewmodels
{

class InputLevelerViewModel final : public shared::controls::dial::DialRuntimeOverlayViewModel
{
public:
    void SetEnabled(bool enabled);
    void ApplyTelemetry(double slow, double fast);
    void ClearTelemetry();
    [[nodiscard]] bool Enabled() const override;
    [[nodiscard]] std::optional<double> DisplayValue() const override;
    [[nodiscard]] std::optional<double> FastPosition() const override;

private:
    std::optional<std::pair<double, double>> telemetry_;
    bool enabled_{};
};

} // namespace consolidator::vst3::features::channelstrip::gainstaging::viewmodels
