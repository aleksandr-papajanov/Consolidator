#pragma once

namespace consolidator::settings {

class SaturatorOptions final {
public:
    static constexpr double MinimumSaturation = 0.0;
    static constexpr double MaximumSaturation = 1.0;
    static constexpr double DefaultSaturation = 0.0;
    static constexpr double MaximumDriveDb = 24.0;
    static constexpr double MinimumOutputDb = -24.0;
    static constexpr double MaximumOutputDb = 24.0;
    static constexpr double DefaultOutputDb = 0.0;
};

} // namespace consolidator::settings
