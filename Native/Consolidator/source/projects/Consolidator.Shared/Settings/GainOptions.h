#pragma once

namespace consolidator::settings {

class GainOptions final {
public:
    static constexpr double MinimumGainDb = -38.0;
    static constexpr double MaximumGainDb = 38.0;
    static constexpr double DefaultGainDb = 0.0;
};

} // namespace consolidator::settings
