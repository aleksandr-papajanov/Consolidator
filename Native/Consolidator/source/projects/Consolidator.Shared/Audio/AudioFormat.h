#pragma once

#include "../Settings/GlobalSettings.h"

#include <cstddef>

namespace consolidator::audio {

struct AudioFormat {
    double sampleRateHz = settings::GlobalSettings::DefaultSampleRateHz;
    std::size_t blockSize = 0;
};

} // namespace consolidator::audio
