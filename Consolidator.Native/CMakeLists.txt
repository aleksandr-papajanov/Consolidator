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
    Source/Core/Commands/Commands.h
    Source/Core/Commands/DspActions.h
    Source/Core/Registry/InstanceRegistry.h
    Source/Core/Registry/InstanceRegistry.cpp
    Source/Core/Logging/Log.h
    Source/Core/Logging/Log.cpp
    Source/Core/Settings/DspDeviceSettings.h
    Source/Dsp/Parameters/ParameterValue.h
    Source/Dsp/Parameters/ParameterAddress.h
    Source/Dsp/Parameters/ParameterChange.h
    Source/Dsp/Parameters/ParameterBatch.h
    Source/Dsp/Parameters/ParameterHelper.h
    Source/Dsp/Processors/IDspDevice.h
    Source/Dsp/Processors/Gain/Gain.h
    Source/Dsp/Processors/Gain/Gain.cpp
    Source/Dsp/Processors/Saturator/Saturator.h
    Source/Dsp/Processors/Saturator/Saturator.cpp
    Source/Dsp/Processors/Saturator/EnvelopeDetector.h
    Source/Dsp/Processors/Saturator/EnvelopeDetector.cpp
    Source/Dsp/Processors/Compressor/Compressor.h
    Source/Dsp/Processors/Compressor/Compressor.cpp
    Source/Dsp/Processors/Compressor/CompressorSidechain.h
    Source/Dsp/Processors/Compressor/CompressorSidechain.cpp
    Source/Dsp/Processors/Compressor/RmsDetector.h
    Source/Dsp/Processors/Compressor/RmsDetector.cpp
    Source/Dsp/Processors/Equalizer/Equalizer.h
    Source/Dsp/Processors/Equalizer/Equalizer.cpp
    Source/Dsp/Processors/Equalizer/Filters/Filter.h
    Source/Dsp/Processors/Equalizer/Filters/Filter.cpp
    Source/Dsp/Processors/Equalizer/Filters/BellFilter.h
    Source/Dsp/Processors/Equalizer/Filters/BellFilter.cpp
    Source/Dsp/Processors/Equalizer/Filters/LowShelfFilter.h
    Source/Dsp/Processors/Equalizer/Filters/LowShelfFilter.cpp
    Source/Dsp/Processors/Equalizer/Filters/HighShelfFilter.h
    Source/Dsp/Processors/Equalizer/Filters/HighShelfFilter.cpp
    Source/Dsp/Processors/Equalizer/Filters/TiltFilter.h
    Source/Dsp/Processors/Equalizer/Filters/TiltFilter.cpp
    Source/Dsp/Processors/Equalizer/Filters/GainFilter.h
    Source/Dsp/Processors/Equalizer/Filters/GainFilter.cpp
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