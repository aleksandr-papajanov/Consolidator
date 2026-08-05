# Final fixes: Equalizer (no auto-ID), DspChainBuilder (ElementId ctors + settings),
# DspChainTest (State instead of getters), CMakeLists (drop FilterSettings, add Core/Settings).
$root = 'd:\Projects\Ableton\Consolidator\Consolidator.Native'

function Write-File($path, $content) {
    Set-Content -Path $path -Value $content -Encoding utf8
    Write-Host "OK: $path"
}

# --- Equalizer.cpp (AddFilter no longer overrides ElementId) ---
Write-File "$root\Source\Dsp\Processors\Equalizer.cpp" @'
#include "Dsp/Processors/Equalizer.h"

namespace consolidator::dsp
{

void Equalizer::Process(const double* input,
                        double* output,
                        std::size_t frameCount,
                        std::size_t channelCount)
{
    if (filters_.empty())
    {
        const auto sampleCount = frameCount * channelCount;
        for (std::size_t i = 0; i < sampleCount; ++i)
        {
            output[i] = input[i];
        }
        return;
    }

    const auto sampleCount = frameCount * channelCount;
    for (std::size_t i = 0; i < sampleCount; ++i)
    {
        double sample = input[i];
        for (auto& filter : filters_)
        {
            sample = filter->ProcessSample(sample);
        }
        output[i] = sample;
    }
}

void Equalizer::ApplyParameterChange(const ParameterChange& change)
{
    if (change.address.GetElement() != ElementId::None)
    {
        if (auto* filter = FindFilter(change.address.GetElement()))
        {
            filter->ApplyParameterChange(change);
        }
        return;
    }

    for (auto& filter : filters_)
    {
        filter->ApplyParameterChange(change);
    }
}

void Equalizer::AddFilter(std::unique_ptr<Filter> filter)
{
    if (!filter)
    {
        return;
    }

    filters_.push_back(std::move(filter));
}

Filter* Equalizer::GetFilter(std::size_t index) noexcept
{
    return index < filters_.size() ? filters_[index].get() : nullptr;
}

const Filter* Equalizer::GetFilter(std::size_t index) const noexcept
{
    return index < filters_.size() ? filters_[index].get() : nullptr;
}

Filter* Equalizer::FindFilter(ElementId elementId) noexcept
{
    for (auto& filter : filters_)
    {
        if (filter->GetElementId() == elementId)
        {
            return filter.get();
        }
    }
    return nullptr;
}

const Filter* Equalizer::FindFilter(ElementId elementId) const noexcept
{
    for (const auto& filter : filters_)
    {
        if (filter->GetElementId() == elementId)
        {
            return filter.get();
        }
    }
    return nullptr;
}

} // namespace consolidator::dsp
'@

# --- DspChainBuilder.cpp (4 Bell filters with ElementId ctors + global settings) ---
Write-File "$root\Source\Dsp\DspChainBuilder.cpp" @'
#include "Dsp/DspChainBuilder.h"

#include "Dsp/Eq/BellFilter.h"
#include "Dsp/Processors/Compressor.h"
#include "Dsp/Processors/DspChain.h"
#include "Dsp/Processors/Equalizer.h"
#include "Dsp/Processors/Gain.h"
#include "Dsp/Processors/Saturator.h"

#include "Core/Settings/DspDeviceSettings.h"

namespace consolidator::dsp
{

namespace
{

void AddDefaultEqualizer(Equalizer& equalizer)
{
    using namespace consolidator::core::settings;

    equalizer.AddFilter(std::make_unique<BellFilter>(ElementId::Filter1, kBand1FrequencyHz));
    equalizer.AddFilter(std::make_unique<BellFilter>(ElementId::Filter2, kBand2FrequencyHz));
    equalizer.AddFilter(std::make_unique<BellFilter>(ElementId::Filter3, kBand3FrequencyHz));
    equalizer.AddFilter(std::make_unique<BellFilter>(ElementId::Filter4, kBand4FrequencyHz));
}

} // namespace

std::unique_ptr<DspChain> DspChainBuilder::BuildStandardChain() const
{
    auto chain = std::make_unique<DspChain>();
    auto equalizer = std::make_unique<Equalizer>();
    AddDefaultEqualizer(*equalizer);

    // Input Gain
    chain->AddDevice(std::make_unique<Gain>(ElementId::MainInput));
    // Saturator
    chain->AddDevice(std::make_unique<Saturator>());
    // Compressor
    chain->AddDevice(std::make_unique<Compressor>());
    // Equalizer (4 default Bell filters)
    chain->AddDevice(std::move(equalizer));
    // Output Gain
    chain->AddDevice(std::make_unique<Gain>(ElementId::MainOutput));

    return chain;
}

} // namespace consolidator::dsp
'@

# --- DspChainTest.cpp (filter states instead of getters) ---
Write-File "$root\Tests\Dsp\DspChainTest.cpp" @'
#include "Dsp/DspChainBuilder.h"
#include "Dsp/Parameters/ParameterAddress.h"
#include "Dsp/Processors/Compressor.h"
#include "Dsp/Processors/DspChain.h"
#include "Dsp/Processors/Equalizer.h"
#include "Dsp/Processors/Gain.h"
#include "Dsp/Processors/Saturator.h"

#include "Dsp/Eq/BellFilter.h"
#include "Dsp/Eq/HighShelfFilter.h"
#include "Dsp/Eq/LowShelfFilter.h"
#include "Dsp/Eq/TiltFilter.h"

#include <cassert>
#include <array>
#include <cmath>
#include <memory>

using namespace consolidator::dsp;

namespace
{

void AssertEqualFilterState(const Equalizer& equalizer, std::size_t index,
                            float frequency, float q, float gainDb)
{
    const auto* filter = equalizer.GetFilter(index);
    assert(filter != nullptr);

    const auto* bell = dynamic_cast<const BellFilter*>(filter);
    if (bell)
    {
        const auto& state = bell->GetState();
        assert(state.frequency == frequency);
        assert(state.q == q);
        assert(state.gainDb == gainDb);
        return;
    }

    const auto* lowShelf = dynamic_cast<const LowShelfFilter*>(filter);
    if (lowShelf)
    {
        const auto& state = lowShelf->GetState();
        assert(state.frequency == frequency);
        assert(state.gainDb == gainDb);
        return;
    }

    const auto* highShelf = dynamic_cast<const HighShelfFilter*>(filter);
    if (highShelf)
    {
        const auto& state = highShelf->GetState();
        assert(state.frequency == frequency);
        assert(state.gainDb == gainDb);
        return;
    }

    const auto* tilt = dynamic_cast<const TiltFilter*>(filter);
    if (tilt)
    {
        const auto& state = tilt->GetState();
        assert(state.pivot == frequency);
        assert(state.gainDb == gainDb);
        return;
    }

    assert(false); // unexpected filter type
}

} // namespace

int main()
{
    DspChainBuilder builder;
    auto chain = builder.BuildStandardChain();
    assert(chain != nullptr);
    assert(chain->GetDeviceCount() == 5);

    constexpr std::size_t frameCount = 8;
    constexpr std::size_t channelCount = 2;
    constexpr std::size_t sampleCount = frameCount * channelCount;

    std::array<double, sampleCount> input{};
    std::array<double, sampleCount> interim{};
    std::array<double, sampleCount> output{};

    for (std::size_t i = 0; i < sampleCount; ++i)
    {
        input[i] = static_cast<double>(i) * 0.1;
    }

    chain->Process(input.data(), interim.data(), output.data(), frameCount, channelCount);
    for (std::size_t i = 0; i < sampleCount; ++i)
    {
        assert(std::abs(output[i] - input[i]) < 1e-9);
    }

    const ParameterChange gainChange{
        ParameterAddress{DeviceKind::Gain, ElementId::MainInput, ParameterId::Gain},
        ParameterValue{12.0f}
    };
    chain->ApplyParameterChange(gainChange);

    chain->Process(input.data(), interim.data(), output.data(), frameCount, channelCount);

    const double expectedFactor = std::pow(10.0, 12.0 / 20.0);
    for (std::size_t i = 0; i < sampleCount; ++i)
    {
        assert(std::abs(output[i] - input[i] * expectedFactor) < 1e-9);
    }

    const ParameterChange bypassChange{
        ParameterAddress{DeviceKind::Gain, ElementId::MainInput, ParameterId::Bypass},
        ParameterValue{true}
    };
    chain->ApplyParameterChange(bypassChange);

    chain->Process(input.data(), interim.data(), output.data(), frameCount, channelCount);
    for (std::size_t i = 0; i < sampleCount; ++i)
    {
        assert(std::abs(output[i] - input[i]) < 1e-9);
    }

    // Equalizer is a proxy over filters; each filter owns its ElementId.
    const auto* equalizer = static_cast<const Equalizer*>(chain->GetDevice(3));
    assert(equalizer != nullptr);
    assert(equalizer->GetFilterCount() == 4);
    AssertEqualFilterState(*equalizer, 0, 1000.0f, 0.707f, 0.0f);

    const ParameterChange freqChange{
        ParameterAddress{DeviceKind::Equalizer, ElementId::Filter3, ParameterId::Frequency},
        ParameterValue{1200.0f}
    };
    chain->ApplyParameterChange(freqChange);

    chain->Process(input.data(), interim.data(), output.data(), frameCount, channelCount);

    AssertEqualFilterState(*equalizer, 2, 1200.0f, 0.707f, 0.0f); // Filter3 updated
    AssertEqualFilterState(*equalizer, 0, 1000.0f, 0.707f, 0.0f); // others untouched
    AssertEqualFilterState(*equalizer, 1, 2000.0f, 0.707f, 0.0f);
    AssertEqualFilterState(*equalizer, 3, 4000.0f, 0.707f, 0.0f);

    ParameterBatch batch;
    batch.Add(ParameterChange{
        ParameterAddress{DeviceKind::Saturator, ElementId::None, ParameterId::Drive},
        ParameterValue{2.0f}
    });
    batch.Add(ParameterChange{
        ParameterAddress{DeviceKind::Compressor, ElementId::None, ParameterId::Threshold},
        ParameterValue{-18.0f}
    });
    chain->ApplyParameterBatch(batch);

    chain->Process(input.data(), interim.data(), output.data(), frameCount, channelCount);

    const auto& satState = static_cast<const Saturator*>(chain->GetDevice(1))->GetState();
    assert(satState.drive == 2.0f);

    const auto& compState = static_cast<const Compressor*>(chain->GetDevice(2))->GetState();
    assert(compState.thresholdDb == -18.0f);

    return 0;
}
'@

# --- CMakeLists.txt (drop FilterSettings.h, add Core/Settings) ---
Write-File "$root\CMakeLists.txt" @'
cmake_minimum_required(VERSION 3.20)

project(Consolidator.Native
    VERSION 0.1.0
    DESCRIPTION "Consolidator native DSP and Core processing library"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Min API externals require static CRT - keep the whole project consistent.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Dev mode: enables runtime logging via CONSOLIDATOR_LOG macro.
option(CONSOLIDATOR_DEV "Enable development-mode logging" OFF)
if(CONSOLIDATOR_DEV)
    add_compile_definitions(CONSOLIDATOR_DEV)
    message(STATUS "CONSOLIDATOR_DEV=ON - runtime logging enabled")
endif()

# ---------------------------------------------------------------------------
# ConsolidatorCore - static library, no Max/Min/API dependencies
# ---------------------------------------------------------------------------
add_library(ConsolidatorCore STATIC)

target_sources(ConsolidatorCore PRIVATE
    Source/Core/Instance/ConsolidatorInstance.h
    Source/Core/Instance/ConsolidatorInstance.cpp
    Source/Core/Instance/InstanceId.h
    Source/Core/Groups/GroupId.h
    Source/Core/Groups/InstanceGroup.h
    Source/Core/Commands/ParameterChange.h
    Source/Core/Registry/InstanceRegistry.h
    Source/Core/Registry/InstanceRegistry.cpp
    Source/Core/Logging/Log.h
    Source/Core/Logging/Log.cpp
    Source/Core/Settings/DspDeviceSettings.h
    Source/Dsp/Parameters/ParameterValue.h
    Source/Dsp/Parameters/ParameterAddress.h
    Source/Dsp/Parameters/ParameterChange.h
    Source/Dsp/Parameters/ParameterBatch.h
    Source/Dsp/Processors/IDspDevice.h
    Source/Dsp/Processors/Gain.h
    Source/Dsp/Processors/Gain.cpp
    Source/Dsp/Processors/Saturator.h
    Source/Dsp/Processors/Saturator.cpp
    Source/Dsp/Processors/Compressor.h
    Source/Dsp/Processors/Compressor.cpp
    Source/Dsp/Processors/Equalizer.h
    Source/Dsp/Processors/Equalizer.cpp
    Source/Dsp/Eq/Filter.h
    Source/Dsp/Eq/Filter.cpp
    Source/Dsp/Eq/BellFilter.h
    Source/Dsp/Eq/BellFilter.cpp
    Source/Dsp/Eq/LowShelfFilter.h
    Source/Dsp/Eq/LowShelfFilter.cpp
    Source/Dsp/Eq/HighShelfFilter.h
    Source/Dsp/Eq/HighShelfFilter.cpp
    Source/Dsp/Eq/TiltFilter.h
    Source/Dsp/Eq/TiltFilter.cpp
    Source/Dsp/Eq/GainFilter.h
    Source/Dsp/Eq/GainFilter.cpp
    Source/Dsp/Processors/DspChain.h
    Source/Dsp/Processors/DspChain.cpp
    Source/Dsp/DspChainBuilder.h
    Source/Dsp/DspChainBuilder.cpp
)

target_include_directories(ConsolidatorCore PUBLIC
    Source
)

target_compile_features(ConsolidatorCore PUBLIC cxx_std_20)

# ---------------------------------------------------------------------------
# ConsolidatorMax - Min API external, thin adapter over Core
# ---------------------------------------------------------------------------
add_subdirectory(Source/Max/External)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()

add_executable(ConsolidatorInstanceTest
    Tests/Core/ConsolidatorInstanceTest.cpp
)

target_include_directories(ConsolidatorInstanceTest PRIVATE Source)
target_link_libraries(ConsolidatorInstanceTest PRIVATE ConsolidatorCore)
target_compile_features(ConsolidatorInstanceTest PRIVATE cxx_std_20)

add_executable(InstanceRegistryTest
    Tests/Core/InstanceRegistryTest.cpp
)

target_include_directories(InstanceRegistryTest PRIVATE Source)
target_link_libraries(InstanceRegistryTest PRIVATE ConsolidatorCore)
target_compile_features(InstanceRegistryTest PRIVATE cxx_std_20)

add_executable(DspChainTest
    Tests/Dsp/DspChainTest.cpp
)

target_include_directories(DspChainTest PRIVATE Source)
target_link_libraries(DspChainTest PRIVATE ConsolidatorCore)
target_compile_features(DspChainTest PRIVATE cxx_std_20)

include(CTest)
add_test(NAME ConsolidatorInstanceTest COMMAND ConsolidatorInstanceTest)
add_test(NAME InstanceRegistryTest COMMAND InstanceRegistryTest)
add_test(NAME DspChainTest COMMAND DspChainTest)
'@

Write-Host "Final EQ files restored."