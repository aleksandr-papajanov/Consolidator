#pragma once

namespace consolidator::settings {

struct FitOptions final {
    static constexpr double CaptureSeconds = 4.0;
    static constexpr double WarmupSeconds = 0.5;
    static constexpr double InitialStep = 0.1;
    static constexpr double RelativeFunctionTolerance = 1e-5;
    static constexpr double RelativeParameterTolerance = 1e-3;
    static constexpr int MaximumEvaluations = 500;
};

} // namespace consolidator::settings
