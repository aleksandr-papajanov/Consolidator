#pragma once

#include "../../../Settings/AudioOptions.h"
#include "../../../Settings/EqOptions.h"

namespace consolidator::dsp {

struct GainFilterSettings {
    double gainDb = 0.0;
    double sampleRate = settings::AudioOptions::DefaultSampleRateHz;
};

struct BellFilterSettings {
    double frequencyHz = settings::EqOptions::DefaultFrequencyHz;
    double q = 1.0;
    double gainDb = 0.0;
    double sampleRate = settings::AudioOptions::DefaultSampleRateHz;
};

struct LowShelfFilterSettings {
    double frequencyHz = settings::EqOptions::DefaultFrequencyHz;
    double q = 0.25;
    double gainDb = 0.0;
    double sampleRate = settings::AudioOptions::DefaultSampleRateHz;
};

struct HighShelfFilterSettings {
    double frequencyHz = settings::EqOptions::DefaultFrequencyHz;
    double q = 0.25;
    double gainDb = 0.0;
    double sampleRate = settings::AudioOptions::DefaultSampleRateHz;
};

struct TiltFilterSettings {
    double pivotHz = settings::EqOptions::DefaultFrequencyHz;
    double q = 0.25;
    double gainDb = 0.0;
    double sampleRate = settings::AudioOptions::DefaultSampleRateHz;
};

} // namespace consolidator::dsp
