#pragma once

namespace consolidator::settings {

class CompressorOptions final {
public:
    static constexpr double MinimumAttackMs = 0.1;
    static constexpr double MaximumAttackMs = 500.0;
    static constexpr double DefaultAttackMs = 10.0;
    static constexpr double MinimumReleaseMs = 5.0;
    static constexpr double MaximumReleaseMs = 2000.0;
    static constexpr double DefaultReleaseMs = 100.0;
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
    static constexpr double ProcessingThresholdDb = -18.0;
    static constexpr double FixedRatio = 4.0;
};

} // namespace consolidator::settings
