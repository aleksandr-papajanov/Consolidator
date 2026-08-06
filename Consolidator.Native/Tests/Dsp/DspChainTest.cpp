#include "Dsp/DspChainBuilder.h"
#include "Dsp/Parameters/ParameterAddress.h"
#include "Dsp/Processors/Compressor/Compressor.h"
#include "Dsp/Processors/DspChain.h"
#include "Dsp/Processors/Equalizer/Equalizer.h"
#include "Dsp/Processors/Gain/Gain.h"
#include "Dsp/Processors/Saturator/Saturator.h"

#include "Dsp/Processors/Equalizer/Filters/BellFilter.h"
#include "Dsp/Processors/Equalizer/Filters/GainFilter.h"
#include "Dsp/Processors/Equalizer/Filters/LowShelfFilter.h"

#include <cassert>
#include <array>
#include <memory>

using namespace consolidator::dsp;

int main()
{
    DspChainBuilder builder;
    auto chain = builder.BuildStandardChain();
    assert(chain != nullptr);
    // Chain: InputGain(0) → Saturator(1) → Compressor(2) → EQ Bank0..6(3-9) → OutputGain(10)
    assert(chain->GetDeviceCount() == 11);

    constexpr std::size_t frameCount = 8;
    constexpr std::size_t channelCount = 2;
    constexpr std::size_t sampleCount = frameCount * channelCount;

    std::array<double, sampleCount> input{};
    std::array<double, sampleCount> interim{};
    std::array<double, sampleCount> output{};

    // Smoke: process should not crash
    chain->Process(input.data(), interim.data(), output.data(), frameCount, channelCount);

    // Equalizer Bank0 is at index 3
    const auto* equalizer = static_cast<const Equalizer*>(chain->GetDevice(3));
    assert(equalizer != nullptr);
    assert(equalizer->GetBankId() == BankId::Bank0);
    assert(equalizer->GetFilterCount() == 7);

    // Band 0: GainFilter
    const auto* gainFilter = dynamic_cast<const GainFilter*>(equalizer->GetFilter(0));
    assert(gainFilter != nullptr);
    assert(gainFilter->GetElementKind() == detail::ElementKind::EqFilter);
    assert(gainFilter->GetElementIndex() == 0);

    // Band 4: BellFilter at 1000 Hz
    const auto* bell0 = dynamic_cast<const BellFilter*>(equalizer->GetFilter(4));
    assert(bell0 != nullptr);
    assert(bell0->GetParameters().frequencyHz == 1000.0);
    assert(bell0->GetElementKind() == detail::ElementKind::EqFilter);
    assert(bell0->GetElementIndex() == 4);

    // Change Filter3 frequency on Bank0 — should NOT affect other banks or filters
    const ParameterChange freqChange{
        ParameterAddress::EqFilterFrequency(BankId::Bank0, EqFilterId::Filter3),
        ParameterValue{1200.0f}
    };
    chain->ApplyParameterChange(freqChange);
    chain->Process(input.data(), interim.data(), output.data(), frameCount, channelCount);

    const auto* lowShelf = dynamic_cast<const LowShelfFilter*>(equalizer->GetFilter(2));
    assert(lowShelf != nullptr);
    assert(lowShelf->GetParameters().frequencyHz == 1200.0);
    assert(lowShelf->GetElementKind() == detail::ElementKind::EqFilter);
    assert(lowShelf->GetElementIndex() == 2);
    assert(bell0->GetParameters().frequencyHz == 1000.0); // untouched

    // Batch: Saturator drive + Compressor threshold
    ParameterBatch batch;
    batch.Add(ParameterChange{
        ParameterAddress::SaturatorDrive(),
        ParameterValue{2.0f}
    });
    batch.Add(ParameterChange{
        ParameterAddress::CompressorThreshold(),
        ParameterValue{-18.0f}
    });
    chain->ApplyParameterBatch(batch);
    chain->Process(input.data(), interim.data(), output.data(), frameCount, channelCount);

    assert(static_cast<const Saturator*>(chain->GetDevice(1))->GetState().drive == 2.0f);
    assert(static_cast<const Compressor*>(chain->GetDevice(2))->GetState().thresholdDb == -18.0f);

    return 0;
}