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
# ConsolidatorCore - shared library, no Max/Min/API dependencies
# ---------------------------------------------------------------------------
add_library(ConsolidatorCore SHARED)
set_target_properties(ConsolidatorCore PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_sources(ConsolidatorCore PRIVATE
    Source/Core/Instance/ConsolidatorInstance.h
    Source/Core/Instance/ConsolidatorInstance.cpp
    Source/Core/Instance/ConsolidatorInstance.StateCommandHandler.cpp
    Source/Core/Domain/Ids/InstanceId.h
    Source/Core/Domain/Ids/GroupId.h
    Source/Core/Domain/Ids/DspIds.h
    Source/Core/Instance/Queues/DspUpdateMailbox.h
    Source/Core/Instance/Queues/DspUpdateMailbox.cpp
    Source/Core/Coordinator/InstanceCoordinator.h
    Source/Core/Coordinator/InstanceCoordinator.cpp
    Source/Core/Coordinator/InstanceCoordinator.CommandHandler.cpp
    Source/Core/Coordinator/InstanceCoordinator.DispatchHandler.cpp
    Source/Core/Routing/CommandRouter.h
    Source/Core/Routing/CommandRouter.CommandHandler.cpp
    Source/Core/Routing/CommandRouter.StateCommandHandler.cpp
    Source/Core/Routing/CommandRouter.ReadCommandHandler.cpp
    Source/Core/Routing/CommandRouter.WriteCommandHandler.cpp
    Source/Core/Routing/StateRouter.h
    Source/Core/Routing/StateRouter.cpp
    Source/Core/Routing/ParameterConstraintResolver.h
    Source/Core/Routing/ParameterConstraintResolver.cpp
    Source/Core/Routing/StateResponsePublisher.h
    Source/Core/Routing/StateResponsePublisher.cpp
    Source/Core/Domain/State/InstanceState.h
    Source/Core/Domain/State/StateStore.h
    Source/Core/Domain/State/StateStore.Internal.h
    Source/Core/Domain/State/StateStore.cpp
    Source/Core/Domain/State/StateStore.Gain.cpp
    Source/Core/Domain/State/StateStore.Saturator.cpp
    Source/Core/Domain/State/StateStore.Compressor.cpp
    Source/Core/Domain/State/StateStore.Equalizer.cpp
    Source/Core/Domain/State/StateStore.Read.cpp
    Source/Core/Domain/State/BankState.h
    Source/Core/Domain/State/GainState.h
    Source/Core/Domain/State/SaturatorState.h
    Source/Core/Domain/State/CompressorState.h
    Source/Core/Domain/State/EqualizerState.h
    Source/Core/Domain/State/FilterState.h
    Source/Core/Domain/State/StateProtocol.h
    Source/Core/Domain/Commands/Commands.h
    Source/Core/Domain/Commands/StateProtocolCommands.h
    Source/Core/Queues/ConcurrentQueue.h
    Source/Core/Queues/SpscQueue.h
    Source/Core/Queues/LatestValueMailbox.h
    Source/Core/Registry/InstanceRegistry.h
    Source/Core/Registry/InstanceRegistry.cpp
    Source/Core/Logging/Log.h
    Source/Core/Logging/Log.cpp
    Source/Core/Settings/DspDeviceSettings.h
    Source/Core/Domain/ParameterVariant.h
    Source/Core/Domain/State/ParameterState.h
    Source/Dsp/Utilities/TimeCoefficient.h
    Source/Dsp/Processors/DspDevice.h
    Source/Dsp/Processors/DspDevice.cpp
    Source/Dsp/Processors/Gain/Gain.h
    Source/Dsp/Processors/Gain/Gain.cpp
    Source/Dsp/Processors/Saturator/Saturator.h
    Source/Dsp/Processors/Saturator/Saturator.cpp
    Source/Dsp/Processors/Saturator/DetectorEnvelopeFollower.h
    Source/Dsp/Processors/Saturator/DetectorEnvelopeFollower.cpp
    Source/Dsp/Processors/Compressor/Compressor.h
    Source/Dsp/Processors/Compressor/Compressor.cpp
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

add_executable(StateProtocolIntegrationTest
    Tests/Core/StateProtocolIntegrationTest.cpp
)

target_include_directories(StateProtocolIntegrationTest PRIVATE Source)
target_link_libraries(StateProtocolIntegrationTest PRIVATE ConsolidatorCore)
target_compile_features(StateProtocolIntegrationTest PRIVATE cxx_std_20)

include(CTest)
add_test(NAME ConsolidatorInstanceTest COMMAND ConsolidatorInstanceTest)
add_test(NAME InstanceRegistryTest COMMAND InstanceRegistryTest)
add_test(NAME DspChainTest COMMAND DspChainTest)
add_test(NAME StateProtocolIntegrationTest COMMAND StateProtocolIntegrationTest)
