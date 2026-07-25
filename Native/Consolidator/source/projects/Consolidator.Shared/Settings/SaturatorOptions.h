#pragma once

namespace consolidator::settings {

class SaturatorOptions final {
public:
    static constexpr double MinimumInputDb = -24.0;
    static constexpr double MaximumInputDb = 24.0;
    static constexpr double DefaultInputDb = 0.0;
    static constexpr double MinimumOutputDb = -24.0;
    static constexpr double MaximumOutputDb = 24.0;
    static constexpr double DefaultOutputDb = 0.0;
    static constexpr double MinimumMix = 0.0;
    static constexpr double MaximumMix = 1.0;
    static constexpr double DefaultMix = 1.0;
    static constexpr long ModeCount = 3;
    static constexpr long DefaultMode = 0;
};

} // namespace consolidator::settings
