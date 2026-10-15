#pragma once

#include <optional>
#include <string_view>

namespace consolidator::vst3::shared::controls::dial
{

class DialRuntimeOverlayViewModel
{
public:
    virtual ~DialRuntimeOverlayViewModel() = default;
    [[nodiscard]] virtual bool Enabled() const = 0;
    [[nodiscard]] virtual std::optional<double> DisplayValue() const = 0;
    [[nodiscard]] virtual std::optional<double> FastPosition() const = 0;
};

struct DialRuntimeOverlayBinding
{
    std::string_view path;
    // The feature binding owns the model and outlives parameter controls using it.
    const DialRuntimeOverlayViewModel* model{};
};

} // namespace consolidator::vst3::shared::controls::dial
