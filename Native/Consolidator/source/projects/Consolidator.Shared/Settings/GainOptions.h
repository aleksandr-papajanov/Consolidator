#pragma once

namespace consolidator::settings {

class GainOptions final {
public:
    static constexpr double MinimumGainDb = -15.0;
    static constexpr double MaximumGainDb = 15.0;
    static constexpr double DefaultGainDb = 0.0;
};

} // namespace consolidator::settings
