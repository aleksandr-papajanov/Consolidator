#pragma once

#include <array>

#include "Core/Ids/DspIds.h"

namespace consolidator::core::settings
{

// ---- Ranged value ----

template <typename T>
struct RangedValue
{
    T min;
    T max;
    T defaultValue;
};

// ---- Bypass setting ----

struct BypassSetting
{
    bool defaultValue = false;
};

// ---- Global defaults ----

inline constexpr double kDefaultSampleRate = 48000.0;

namespace FilterDefaults
{
    inline constexpr double kMinFrequencyHz = 20.0;
    inline constexpr double kMaxFrequencyHz = 20000.0;
    inline constexpr double kDefaultFrequencyHz = 1000.0;

    inline constexpr double kMinQ = 0.1;
    inline constexpr double kMaxQ = 10.0;
    inline constexpr double kDefaultQ = 0.707;

    inline constexpr double kMinGainDb = -24.0;
    inline constexpr double kMaxGainDb = 24.0;
    inline constexpr double kDefaultGainDb = 0.0;
} // namespace FilterDefaults

namespace GainDefaults
{
    inline constexpr double kMinGainDb = -60.0;
    inline constexpr double kMaxGainDb = 24.0;
    inline constexpr double kDefaultGainDb = 0.0;
} // namespace GainDefaults

namespace SaturatorDefaults
{
    inline constexpr double kMinDrive = 0.1;
    inline constexpr double kMaxDrive = 10.0;
    inline constexpr double kDefaultDrive = 1.0;

    inline constexpr double kMinOutputDb = -24.0;
    inline constexpr double kMaxOutputDb = 24.0;
    inline constexpr double kDefaultOutputDb = 0.0;

    inline constexpr double kMinMix = 0.0;
    inline constexpr double kMaxMix = 1.0;
    inline constexpr double kDefaultMix = 1.0;
    inline constexpr double kMaximumDetectorAmount = 8.0;
    inline constexpr double kMaximumDriveModulation = 16.0;
} // namespace SaturatorDefaults

namespace DetectorDefaults
{
    inline constexpr double kMinimumTimeMs = 0.01;
    inline constexpr double kDefaultAttackMs = 10.0;
    inline constexpr double kDefaultReleaseMs = 100.0;
    inline constexpr double kDefaultLowShelfFrequencyHz = 100.0;
    inline constexpr double kDefaultBellFrequencyHz = 1000.0;
} // namespace DetectorDefaults

namespace CompressorDefaults
{
    inline constexpr double kMinThresholdDb = -60.0;
    inline constexpr double kMaxThresholdDb = 0.0;
    inline constexpr double kDefaultThresholdDb = -12.0;

    inline constexpr double kMinRatio = 1.0;
    inline constexpr double kMaxRatio = 20.0;
    inline constexpr double kDefaultRatio = 4.0;

    inline constexpr double kMinAttackMs = 0.1;
    inline constexpr double kMaxAttackMs = 100.0;
    inline constexpr double kDefaultAttackMs = 5.0;

    inline constexpr double kMinReleaseMs = 1.0;
    inline constexpr double kMaxReleaseMs = 1000.0;
    inline constexpr double kDefaultReleaseMs = 100.0;

    inline constexpr double kMinOutputDb = -24.0;
    inline constexpr double kMaxOutputDb = 24.0;
    inline constexpr double kDefaultOutputDb = 0.0;

    inline constexpr double kMinMix = 0.0;
    inline constexpr double kMaxMix = 1.0;
    inline constexpr double kDefaultMix = 1.0;

    inline constexpr double kSoftKneeWidthDb = 6.0;
    inline constexpr double kMinimumLevelLinear = 1.0e-12;
    inline constexpr double kMinimumGainReductionDb = -60.0;
} // namespace CompressorDefaults

// ---- Filter kind ----

enum class FilterKind
{
    Bell,
    LowShelf,
    HighShelf,
    Tilt,
    GainFilter
};

// ---- Filter settings (one EQ band) ----

struct FilterSettings
{
    dsp::FilterId elementId = dsp::FilterId::Filter1;
    FilterKind kind = FilterKind::Bell;
    RangedValue<double> frequencyHz{
        FilterDefaults::kMinFrequencyHz,
        FilterDefaults::kMaxFrequencyHz,
        FilterDefaults::kDefaultFrequencyHz
    };
    RangedValue<double> q{
        FilterDefaults::kMinQ,
        FilterDefaults::kMaxQ,
        FilterDefaults::kDefaultQ
    };
    RangedValue<double> gainDb{
        FilterDefaults::kMinGainDb,
        FilterDefaults::kMaxGainDb,
        FilterDefaults::kDefaultGainDb
    };
    BypassSetting bypass{};
};

// ---- Equalizer settings (7 bands) ----

struct EqualizerSettings
{
    static inline constexpr std::size_t kBandCount = 7;

    dsp::BankId bankId = dsp::BankId::Bank0;

    std::array<FilterSettings, kBandCount> bands
    {{
        {
            dsp::FilterId::Filter1,
            FilterKind::GainFilter,
            { FilterDefaults::kMinFrequencyHz, FilterDefaults::kMaxFrequencyHz, FilterDefaults::kDefaultFrequencyHz },
            { FilterDefaults::kMinQ, FilterDefaults::kMaxQ, FilterDefaults::kDefaultQ },
            { FilterDefaults::kMinGainDb, FilterDefaults::kMaxGainDb, FilterDefaults::kDefaultGainDb },
            BypassSetting{false}
        },
        {
            dsp::FilterId::Filter2,
            FilterKind::Tilt,
            { FilterDefaults::kMinFrequencyHz, FilterDefaults::kMaxFrequencyHz, 1000.0 },
            { FilterDefaults::kMinQ, FilterDefaults::kMaxQ, FilterDefaults::kDefaultQ },
            { FilterDefaults::kMinGainDb, FilterDefaults::kMaxGainDb, FilterDefaults::kDefaultGainDb },
            BypassSetting{false}
        },
        {
            dsp::FilterId::Filter3,
            FilterKind::LowShelf,
            { FilterDefaults::kMinFrequencyHz, FilterDefaults::kMaxFrequencyHz, 100.0 },
            { FilterDefaults::kMinQ, FilterDefaults::kMaxQ, FilterDefaults::kDefaultQ },
            { FilterDefaults::kMinGainDb, FilterDefaults::kMaxGainDb, FilterDefaults::kDefaultGainDb },
            BypassSetting{false}
        },
        {
            dsp::FilterId::Filter4,
            FilterKind::HighShelf,
            { FilterDefaults::kMinFrequencyHz, FilterDefaults::kMaxFrequencyHz, 10000.0 },
            { FilterDefaults::kMinQ, FilterDefaults::kMaxQ, FilterDefaults::kDefaultQ },
            { FilterDefaults::kMinGainDb, FilterDefaults::kMaxGainDb, FilterDefaults::kDefaultGainDb },
            BypassSetting{false}
        },
        {
            dsp::FilterId::Filter5,
            FilterKind::Bell,
            { FilterDefaults::kMinFrequencyHz, FilterDefaults::kMaxFrequencyHz, 1000.0 },
            { FilterDefaults::kMinQ, FilterDefaults::kMaxQ, FilterDefaults::kDefaultQ },
            { FilterDefaults::kMinGainDb, FilterDefaults::kMaxGainDb, FilterDefaults::kDefaultGainDb },
            BypassSetting{false}
        },
        {
            dsp::FilterId::Filter6,
            FilterKind::Bell,
            { FilterDefaults::kMinFrequencyHz, FilterDefaults::kMaxFrequencyHz, 2000.0 },
            { FilterDefaults::kMinQ, FilterDefaults::kMaxQ, FilterDefaults::kDefaultQ },
            { FilterDefaults::kMinGainDb, FilterDefaults::kMaxGainDb, FilterDefaults::kDefaultGainDb },
            BypassSetting{false}
        },
        {
            dsp::FilterId::Filter7,
            FilterKind::Bell,
            { FilterDefaults::kMinFrequencyHz, FilterDefaults::kMaxFrequencyHz, 4000.0 },
            { FilterDefaults::kMinQ, FilterDefaults::kMaxQ, FilterDefaults::kDefaultQ },
            { FilterDefaults::kMinGainDb, FilterDefaults::kMaxGainDb, FilterDefaults::kDefaultGainDb },
            BypassSetting{false}
        }
    }};
};

// ---- Gain settings ----

struct GainSettings
{
    dsp::DeviceId elementId = dsp::DeviceId::MainInputGain;
    RangedValue<double> gainDb{
        GainDefaults::kMinGainDb,
        GainDefaults::kMaxGainDb,
        GainDefaults::kDefaultGainDb
    };
    BypassSetting bypass{};
};

// ---- Detector filter settings ----

struct DetectorFilterSettings
{
    static inline constexpr std::size_t kBandCount = 2;

    std::array<FilterSettings, kBandCount> bands
    {{
        {
            dsp::FilterId::Filter1,
            FilterKind::LowShelf,
            { FilterDefaults::kMinFrequencyHz, FilterDefaults::kMaxFrequencyHz, 100.0 },
            { FilterDefaults::kMinQ, FilterDefaults::kMaxQ, FilterDefaults::kDefaultQ },
            { FilterDefaults::kMinGainDb, FilterDefaults::kMaxGainDb, FilterDefaults::kDefaultGainDb },
            BypassSetting{false}
        },
        {
            dsp::FilterId::Filter1,
            FilterKind::Bell,
            { FilterDefaults::kMinFrequencyHz, FilterDefaults::kMaxFrequencyHz, 1000.0 },
            { FilterDefaults::kMinQ, FilterDefaults::kMaxQ, FilterDefaults::kDefaultQ },
            { FilterDefaults::kMinGainDb, FilterDefaults::kMaxGainDb, FilterDefaults::kDefaultGainDb },
            BypassSetting{false}
        }
    }};
};

// ---- Saturator settings ----

struct SaturatorSettings
{
    RangedValue<double> drive{
        SaturatorDefaults::kMinDrive,
        SaturatorDefaults::kMaxDrive,
        SaturatorDefaults::kDefaultDrive
    };
    RangedValue<double> outputDb{
        SaturatorDefaults::kMinOutputDb,
        SaturatorDefaults::kMaxOutputDb,
        SaturatorDefaults::kDefaultOutputDb
    };
    RangedValue<double> mix{
        SaturatorDefaults::kMinMix,
        SaturatorDefaults::kMaxMix,
        SaturatorDefaults::kDefaultMix
    };
    BypassSetting bypass{};
    DetectorFilterSettings detector{};
};

// ---- Compressor settings ----

struct CompressorSettings
{
    RangedValue<double> thresholdDb{
        CompressorDefaults::kMinThresholdDb,
        CompressorDefaults::kMaxThresholdDb,
        CompressorDefaults::kDefaultThresholdDb
    };
    RangedValue<double> ratio{
        CompressorDefaults::kMinRatio,
        CompressorDefaults::kMaxRatio,
        CompressorDefaults::kDefaultRatio
    };
    RangedValue<double> attackMs{
        CompressorDefaults::kMinAttackMs,
        CompressorDefaults::kMaxAttackMs,
        CompressorDefaults::kDefaultAttackMs
    };
    RangedValue<double> releaseMs{
        CompressorDefaults::kMinReleaseMs,
        CompressorDefaults::kMaxReleaseMs,
        CompressorDefaults::kDefaultReleaseMs
    };
    RangedValue<double> outputDb{
        CompressorDefaults::kMinOutputDb,
        CompressorDefaults::kMaxOutputDb,
        CompressorDefaults::kDefaultOutputDb
    };
    RangedValue<double> mix{
        CompressorDefaults::kMinMix,
        CompressorDefaults::kMaxMix,
        CompressorDefaults::kDefaultMix
    };
    BypassSetting bypass{};
    DetectorFilterSettings detector{};
};

// ---- Aggregate DSP settings (7 banks) ----

struct DspSettings
{
    GainSettings inputGain{ dsp::DeviceId::MainInputGain };
    SaturatorSettings saturator{};
    CompressorSettings compressor{};

    static inline constexpr std::size_t kBankCount = 7;

    std::array<EqualizerSettings, kBankCount> banks
    {{
        EqualizerSettings{ dsp::BankId::Bank0 },
        EqualizerSettings{ dsp::BankId::Bank1 },
        EqualizerSettings{ dsp::BankId::Bank2 },
        EqualizerSettings{ dsp::BankId::Bank3 },
        EqualizerSettings{ dsp::BankId::Bank4 },
        EqualizerSettings{ dsp::BankId::Bank5 },
        EqualizerSettings{ dsp::BankId::Bank6 }
    }};

    GainSettings outputGain{ dsp::DeviceId::MainOutputGain };
};

} // namespace consolidator::core::settings
