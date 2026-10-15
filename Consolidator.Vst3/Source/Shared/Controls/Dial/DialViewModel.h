#pragma once

#include <optional>
#include <string>
#include <utility>

#include "Shared/Controls/Dial/DialRuntimeOverlayViewModel.h"
#include "Shared/Presentation/StateValueViewModel.h"

namespace consolidator::vst3::shared::controls::dial
{

class DialViewModel final
{
public:
    DialViewModel(const shared::presentation::StateValueViewModel& state,
        const DialRuntimeOverlayViewModel* runtimeOverlay);
    void Validate() const;
    void SetScope(bool groupScope);
    void SetPreview(double value);
    void FinishPreview();
    [[nodiscard]] const shared::presentation::StateValueViewModel& State() const;
    [[nodiscard]] std::pair<double, double> EditingRange() const;
    [[nodiscard]] std::pair<double, double> RingRange() const;
    [[nodiscard]] bool Editable() const;
    [[nodiscard]] bool Automated() const;
    [[nodiscard]] bool GroupScope() const;
    [[nodiscard]] bool Previewing() const;
    [[nodiscard]] double Normalize(double value) const;
    [[nodiscard]] double Denormalize(double value) const;
    [[nodiscard]] double Quantize(double normalizedPosition, bool rough) const;
    [[nodiscard]] double DisplayValue() const;
    [[nodiscard]] double RingValue() const;
    [[nodiscard]] std::optional<double> FastPosition() const;
    [[nodiscard]] std::string ValueText() const;

private:
    const shared::presentation::StateValueViewModel& state_;
    const DialRuntimeOverlayViewModel* runtimeOverlay_;
    std::optional<double> preview_;
    std::string suffix_;
    double displayScale_{ 1 };
    int decimals_{ 1 };
    bool logarithmic_{};
    bool decibels_{};
    bool groupScope_{};
};

} // namespace consolidator::vst3::shared::controls::dial
