#pragma once

namespace consolidator::settings {

class CompressorOptions final {
public:
    static constexpr double DefaultAttackMs = 10.0;
    static constexpr double DefaultReleaseMs = 100.0;
    static constexpr double DefaultThresholdDb = -18.0;
    static constexpr double DefaultOutputDb = 0.0;
    static constexpr double DefaultMix = 1.0;
    static constexpr double FixedRatio = 4.0;
};

} // namespace consolidator::settings
