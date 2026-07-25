#pragma once

#include <cmath>

namespace consolidator::audio {

struct GainLevelMetrics final {
    double inputPreDb = 0.0;
    double inputPostDb = 0.0;
    double outputPreDb = 0.0;
    double outputPostDb = 0.0;
    double referenceDb = 0.0;

    bool IsFinite() const noexcept {
        return std::isfinite(inputPreDb) && std::isfinite(inputPostDb) &&
            std::isfinite(outputPreDb) && std::isfinite(outputPostDb) &&
            std::isfinite(referenceDb);
    }
};

} // namespace consolidator::audio
