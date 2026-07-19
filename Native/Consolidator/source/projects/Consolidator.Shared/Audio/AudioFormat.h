#pragma once

#include "../Settings/AudioOptions.h"

#include <cstddef>

namespace consolidator::audio {

struct AudioFormat {
    double sampleRateHz = settings::AudioOptions::DefaultSampleRateHz;
    std::size_t blockSize = 0;
};

} // namespace consolidator::audio
