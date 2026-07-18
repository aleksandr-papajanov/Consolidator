#pragma once

#include "../../../Settings/GlobalSettings.h"

namespace consolidator::dsp {

struct GainFilterSettings {
    double gainDb = 0.0;
};

struct BellFilterSettings {
    double frequencyHz = settings::GlobalSettings::DefaultFrequencyHz;
    double q = 1.0;
    double gainDb = 0.0;
    double sampleRate = settings::GlobalSettings::DefaultSampleRateHz;
};

struct LowShelfFilterSettings {
    double frequencyHz = settings::GlobalSettings::DefaultFrequencyHz;
    double q = settings::GlobalSettings::DefaultFilterQ;
    double gainDb = 0.0;
    double sampleRate = settings::GlobalSettings::DefaultSampleRateHz;
};

struct HighShelfFilterSettings {
    double frequencyHz = settings::GlobalSettings::DefaultFrequencyHz;
    double q = settings::GlobalSettings::DefaultFilterQ;
    double gainDb = 0.0;
    double sampleRate = settings::GlobalSettings::DefaultSampleRateHz;
};

struct TiltFilterSettings {
    double pivotHz = settings::GlobalSettings::DefaultFrequencyHz;
    double q = settings::GlobalSettings::DefaultFilterQ;
    double gainDb = 0.0;
    double sampleRate = settings::GlobalSettings::DefaultSampleRateHz;
};

} // namespace consolidator::dsp
