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
    Source/Core/Ids/InstanceId.h
    Source/Core/Ids/GroupId.h
    Source/Core/Ids/DspIds.h
    Source/Core/Instance/Handlers/StateCommandHandler.h
    Source/Core/Instance/Handlers/StateCommandHandler.cpp
    Source/Core/Instance/Queues/InstanceCommandQueue.h
    Source/Core/Instance/Queues/InstanceCommandQueue.cpp
    Source/Core/Instance/Queues/InstanceResponseQueue.h
    Source/Core/Instance/Queues/InstanceResponseQueue.cpp
    Source/Core/Coordinator/InstanceCoordinator.h
    Source/Core/Coordinator/InstanceCoordinator.cpp
    Source/Core/Coordinator/Delivery/CommandDeliveryQueue.h
    Source/Core/Coordinator/Delivery/CommandDeliveryQueue.cpp
    Source/Core/Coordinator/Routing/CommandRouter.h
    Source/Core/Coordinator/Routing/CommandRouterTopology.cpp
    Source/Core/Coordinator/Routing/CommandRouterDelivery.cpp
    Source/Core/Coordinator/Routing/CommandRouterDispatch.cpp
    Source/Core/Coordinator/Routing/CommandRouterGroup.cpp
    Source/Core/Coordinator/Routing/CommandRouterPlan.cpp
    Source/Core/Coordinator/Routing/CommandRouterRoute.cpp
    Source/Core/Coordinator/Routing/StateRouter.h
    Source/Core/Coordinator/Routing/StateRouter.cpp
    Source/Core/Coordinator/Routing/StateResponsePublisher.h
    Source/Core/Coordinator/Routing/StateResponsePublisher.cpp
    Source/Core/State/InstanceState.h
    Source/Core/State/BankState.h
    Source/Core/State/GainState.h
    Source/Core/State/SaturatorState.h
    Source/Core/State/CompressorState.h
    Source/Core/State/EqualizerState.h
    Source/Core/State/FilterState.h
    Source/Core/State/StateProtocol.h
    Source/Core/Commands/Commands.h
    Source/Core/Commands/ConcurrentQueue.h
    Source/Core/Commands/SpscQueue.h
    Source/Core/Commands/DspActions.h
    Source/Core/Notifications/Notifications.h
    Source/Core/Registry/InstanceRegistry.h
    Source/Core/Registry/InstanceRegistry.cpp
    Source/Core/Logging/Log.h
    Source/Core/Logging/Log.cpp
    Source/Core/Settings/DspDeviceSettings.h
    Source/Core/Parameters/ParameterValue.h
    Source/Core/Parameters/DspParameter.h
    Source/Dsp/Utilities/TimeCoefficient.h
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
