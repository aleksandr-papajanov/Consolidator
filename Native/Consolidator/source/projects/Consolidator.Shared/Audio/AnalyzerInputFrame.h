#pragma once

#include "StereoSample.h"

namespace consolidator::audio {

struct AnalyzerInputFrame {
    StereoSample current;
    StereoSample reference;
};

} // namespace consolidator::audio
