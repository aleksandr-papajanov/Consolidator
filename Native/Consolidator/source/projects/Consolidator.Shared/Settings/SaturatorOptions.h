#pragma once

namespace consolidator::settings {

class SaturatorOptions final {
public:
    static constexpr double MinimumSaturation = 0.0;
    static constexpr double MaximumSaturation = 1.0;
    static constexpr double DefaultSaturation = 0.0;
    static constexpr double MaximumDrive = 10.0;
};

} // namespace consolidator::settings
