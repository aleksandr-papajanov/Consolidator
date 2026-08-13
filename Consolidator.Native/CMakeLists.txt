cmake_minimum_required(VERSION 3.20)

project(Consolidator.Native
    VERSION 0.1.0
    DESCRIPTION "Consolidator native DSP and Core processing library"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

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
    Source/Analysis/Spectrum/SpectrumTypes.h
    Source/Analysis/LatestSnapshot.h
    Source/Analysis/LatestValue.h
    Source/Analysis/TelemetryStream.h
    Source/Analysis/AnalysisService.h
    Source/Analysis/AnalysisView.h
    Source/Analysis/CurveInput.h
    Source/Analysis/AnalysisSlot.h
    Source/Analysis/AnalysisSlot.cpp
    Source/Analysis/Spectrum/SpectrumAnalyzer.h
    Source/Analysis/Spectrum/SpectrumAnalyzer.cpp
    Source/Analysis/Spectrum/SpectrumMapper.h
    Source/Analysis/Spectrum/SpectrumMapper.cpp
    Source/Analysis/Spectrum/AudioWindowAccumulator.h
    Source/Analysis/Spectrum/SpectrumStream.h
    Source/Analysis/Spectrum/SpectrumStream.cpp
    Source/Analysis/FrequencyResponse/FrequencyResponseTypes.h
    Source/Analysis/FrequencyResponse/FrequencyResponseRequestBuilder.h
    Source/Analysis/FrequencyResponse/FrequencyResponseRequestBuilder.cpp
    Source/Analysis/FrequencyResponse/FrequencyResponseCalculator.h
    Source/Analysis/FrequencyResponse/FrequencyResponseCalculator.cpp
    Source/Analysis/FrequencyResponse/FrequencyResponseStream.h
    Source/Analysis/FrequencyResponse/FrequencyResponseStream.cpp
    Source/Analysis/AnalysisService.cpp
    Source/Analysis/AnalysisService.WorkerLoop.cpp
    Source/Analysis/KissFFT/kiss_fft.h
    Source/Analysis/KissFFT/kiss_fftr.h
    Source/Analysis/KissFFT/_kiss_fft_guts.h
    Source/Analysis/KissFFT/kiss_fft_log.h
    Source/Analysis/KissFFT/kiss_fft.c
    Source/Analysis/KissFFT/kiss_fftr.c
    Source/Core/Instance/ConsolidatorInstance.h
    Source/Core/Instance/ConsolidatorInstance.cpp
    Source/Core/Analysis/AnalysisCurveInputBuilder.h
    Source/Core/Analysis/AnalysisCurveInputBuilder.cpp
    Source/Dsp/Telemetry/Telemetry.h
    Source/Core/Instance/ConsolidatorInstance.CommandSubmission.cpp
    Source/Core/Domain/Ids/InstanceId.h
    Source/Core/Domain/Ids/GroupId.h
    Source/Core/Domain/Ids/DspIds.h
    Source/Core/Instance/Queues/RuntimeUpdateMailbox.h
    Source/Core/Instance/Queues/RuntimeUpdateMailbox.cpp
    Source/Core/Instance/Queues/ParameterUpdates.h
    Source/Core/Instance/Queues/RuntimeControlUpdates.h
    Source/Core/Coordinator/InstanceCoordinator.h
    Source/Core/Coordinator/InstanceCoordinator.cpp
    Source/Core/Coordinator/InstanceCoordinator.CommandSubmission.cpp
    Source/Core/Coordinator/InstanceCoordinator.WorkerLoop.cpp
    Source/Core/Routing/CommandRouter.h
    Source/Core/Routing/CommandRouter.CommandHandler.cpp
    Source/Core/Routing/CommandRouter.ActionCommandHandler.cpp
    Source/Core/Routing/CommandRouter.ReadCommandHandler.cpp
    Source/Core/Routing/CommandRouter.WriteCommandHandler.cpp
    Source/Core/Routing/GroupGraph.h
    Source/Core/Routing/GroupGraph.cpp
    Source/Core/Routing/StateRouter.h
    Source/Core/Routing/StateRouter.cpp
    Source/Core/Routing/ParameterConstraintResolver.h
    Source/Core/Routing/ParameterConstraintResolver.cpp
    Source/Core/Routing/StateWriter.h
    Source/Core/Routing/StateWriter.cpp
    Source/Core/Routing/ProcessingStateResolver.h
    Source/Core/Routing/ProcessingStateResolver.cpp
    Source/Core/Routing/ProcessingState.h
    Source/Core/Routing/InstanceAudibilityResolver.h
    Source/Core/Routing/InstanceAudibilityResolver.cpp
    Source/Core/Domain/State/InstanceState.h
    Source/Core/Domain/State/InstanceAudibilityState.h
    Source/Core/Domain/State/StateStore.h
    Source/Core/Domain/State/StateStore.cpp
    Source/Core/Domain/State/StateStore.Factory.cpp
    Source/Core/Domain/State/StateStore.Write.cpp
    Source/Core/Domain/State/StateStore.Read.cpp
    Source/Core/Domain/State/BankState.h
    Source/Core/Domain/State/ChainState.h
    Source/Core/Domain/State/DspStates.h
    Source/Core/Domain/State/StateEntry.h
    Source/Core/Domain/State/StatePath.h
    Source/Core/Domain/Commands/Commands.h
    Source/Core/Domain/Commands/StateProtocolCommands.h
    Source/Core/Domain/Commands/RealtimeCommands.h
    Source/Core/Queues/ConcurrentQueue.h
    Source/Core/Queues/SpscQueue.h
    Source/Core/Queues/LatestValueMailbox.h
    Source/Core/Registry/BankAddress.h
    Source/Core/Registry/InstanceRegistry.h
    Source/Core/Registry/InstanceRegistry.cpp
    Source/Core/Registry/RegistryState.h
    Source/Core/Registry/RegistryState.cpp
    Source/Core/Registry/RegistrySnapshot.h
    Source/Core/Logging/Log.h
    Source/Core/Logging/Log.cpp
    Source/Core/Settings/DspDeviceSettings.h
    Source/Core/Domain/ParameterVariant.h
    Source/Core/Domain/State/ParameterState.h
    Source/Core/Domain/State/StateMarker.h
    Source/Dsp/Utilities/TimeCoefficient.h
    Source/Dsp/Telemetry/MeterSmoother.h
    Source/Dsp/Telemetry/PeakMeter.h
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
    Source/Dsp/Processors/Equalizer/Filters/BiquadDesigner.h
    Source/Dsp/Processors/Equalizer/Filters/BiquadDesigner.cpp
    Source/Dsp/Processors/Equalizer/EqualizerLayout.h
    Source/Dsp/Processors/Equalizer/Filters/Filter.cpp
    Source/Dsp/Processors/Equalizer/Filters/BellFilter.h
    Source/Dsp/Processors/Equalizer/Filters/LowShelfFilter.h
    Source/Dsp/Processors/Equalizer/Filters/HighShelfFilter.h
    Source/Dsp/Processors/Equalizer/Filters/TiltFilter.h
    Source/Dsp/Processors/Equalizer/Filters/TiltFilter.cpp
    Source/Dsp/Processors/Equalizer/Filters/GainFilter.h
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
function(add_consolidator_test target source category)
    add_executable(${target} ${source})
    target_include_directories(${target} PRIVATE Source Tests)
    target_link_libraries(${target} PRIVATE ConsolidatorCore)
    target_compile_features(${target} PRIVATE cxx_std_20)
    add_test(NAME ${target} COMMAND ${target})
    set_tests_properties(${target} PROPERTIES LABELS ${category})
endfunction()

add_consolidator_test(StateModelTests Tests/Component/Core/StateModelTests.cpp component)
add_consolidator_test(RegistryStateTests Tests/Component/Core/RegistryStateTests.cpp component)
add_consolidator_test(StateStoreTests Tests/Component/Core/StateStoreTests.cpp component)
add_consolidator_test(QueueTests Tests/Component/Core/QueueTests.cpp component)
add_consolidator_test(RoutingTests Tests/Component/Core/RoutingTests.cpp component)
add_consolidator_test(ProcessingStateTests Tests/Component/Core/ProcessingStateTests.cpp component)
add_consolidator_test(DspPrimitiveTests Tests/Component/Dsp/DspPrimitiveTests.cpp component)
add_consolidator_test(DspChainTests Tests/Component/Dsp/DspChainTests.cpp component)
add_consolidator_test(AnalysisTests Tests/Component/Analysis/AnalysisTests.cpp component)
add_consolidator_test(ReadCommandTests Tests/Commands/ReadCommandTests.cpp command)
add_consolidator_test(WriteCommandTests Tests/Commands/WriteCommandTests.cpp command)
add_consolidator_test(ActionCommandTests Tests/Commands/ActionCommandTests.cpp command)
add_consolidator_test(StateProtocolIntegrationTests Tests/Integration/StateProtocolIntegrationTests.cpp integration)
add_consolidator_test(InstanceProcessingIntegrationTests Tests/Integration/InstanceProcessingIntegrationTests.cpp integration)
add_consolidator_test(ResponseNotifierLifecycleTests Tests/Integration/ResponseNotifierLifecycleTests.cpp integration)
add_consolidator_test(AnalysisLifecycleTests Tests/Integration/AnalysisLifecycleTests.cpp integration)
