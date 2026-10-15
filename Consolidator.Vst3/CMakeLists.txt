cmake_minimum_required(VERSION 3.22)

project(ConsolidatorPlugin VERSION 0.1.0 LANGUAGES C CXX)

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Consolidator requires Windows x64.")
endif()

set(CONSOLIDATOR_MANAGED_RUNTIME_DIR "" CACHE PATH
    "Directory containing the published Consolidator.Managed.dll and nlopt.dll")
foreach(dependency Consolidator.Managed.dll nlopt.dll)
    if(NOT EXISTS "${CONSOLIDATOR_MANAGED_RUNTIME_DIR}/${dependency}")
        message(FATAL_ERROR "Missing published dependency: ${dependency}. Set CONSOLIDATOR_MANAGED_RUNTIME_DIR.")
    endif()
endforeach()

set(_default_vst3_install_dir "$ENV{CommonProgramW6432}/VST3")
if("$ENV{CommonProgramW6432}" STREQUAL "")
    set(_default_vst3_install_dir "$ENV{CommonProgramFiles}/VST3")
endif()
set(CONSOLIDATOR_VST3_INSTALL_DIR "${_default_vst3_install_dir}" CACHE PATH
    "VST3 directory scanned by the host")

include(FetchContent)
FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(juce)

juce_add_plugin(Consolidator
    COMPANY_NAME "Consolidator"
    BUNDLE_ID "com.consolidator.plugin"
    PLUGIN_MANUFACTURER_CODE Cnsl
    PLUGIN_CODE CsDv
    FORMATS VST3
    PRODUCT_NAME "Consolidator"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(Consolidator)
target_compile_features(Consolidator PRIVATE cxx_std_20)
target_compile_definitions(Consolidator PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    NOMINMAX
    WIN32_LEAN_AND_MEAN)
target_include_directories(Consolidator PRIVATE Source ../Consolidator.Native/External)
target_sources(Consolidator PRIVATE
    Source/Host/Plugin/PluginProcessor.cpp
    Source/Host/Plugin/PluginEditor.cpp
    Source/Shared/Controls/InactiveOverlay.cpp
    Source/Shared/Controls/ActionButton.cpp
    Source/Shared/Theme/UiLookAndFeel.cpp
    Source/Shared/Controls/Parameter/ParameterControl.cpp
    Source/Shared/Controls/Parameter/ParameterButton.cpp
    Source/Features/ChannelStrip/Controls/ChannelStripControl.cpp
    Source/Features/ChannelStrip/Bindings/ChannelStripBindings.cpp
    Source/Shared/Controls/Parameter/Bindings/ParameterBindings.cpp
    Source/Features/Equalizer/Bindings/AnalyzerBindings.cpp
    Source/Features/ChannelStrip/GainStaging/Bindings/GainStagingBindings.cpp
    Source/Shared/Clients/InputTelemetryClient.cpp
    Source/Shared/Presentation/StateValueViewModel.cpp
    Source/Shared/Controls/Parameter/ViewModels/ParameterViewModel.cpp
    Source/Shared/Controls/Dial/DialViewModel.cpp
    Source/Features/ChannelStrip/GainStaging/ViewModels/InputLevelerViewModel.cpp
    Source/Features/Equalizer/ViewModels/AnalyzerViewModel.cpp
    Source/Features/Tracks/ViewModels/TracksViewModel.cpp
    Source/Features/Tracks/ViewModels/TrackFeedback.cpp
    Source/Host/Editor/EditorSession.cpp
    Source/Host/Editor/EditorPresentation.cpp
    Source/Host/Editor/EditorLayout.cpp
    Source/Shared/Controls/Dial/DialControl.cpp
    Source/Features/Equalizer/Presentation/AnalyzerCurveCalculator.cpp
    Source/Features/Equalizer/Controls/EqualizerControl.cpp
    Source/Features/Equalizer/Controls/AnalyzerSourceList.cpp
    Source/Features/Equalizer/Controls/AnalyzerOverlayList.cpp
    Source/Features/Equalizer/Controls/GroupTrackList.cpp
    Source/Features/Equalizer/Controls/AnalyzerControl.cpp
    Source/Features/Equalizer/Controls/AnalyzerRenderer.cpp
    Source/Shared/Editing/DialGestureSession.cpp
    Source/Shared/Clients/RegistryClient.cpp
    Source/Shared/Clients/ProtocolClient.cpp
    Source/Shared/Clients/TargetStateClient.cpp
    Source/Shared/Clients/StateCommands.cpp
    Source/Shared/Clients/AnalyzerClient.cpp
    Source/Shared/Clients/HistoryClient.cpp
    Source/Shared/Clients/TrackLevelClient.cpp
    Source/Features/Tracks/Controls/TrackListControl.cpp
    Source/Features/Equalizer/Controls/BankColumnControl.cpp
    Source/Features/Tracks/Controls/InstanceButtonsControl.cpp
    Source/Features/Tracks/Controls/BankCellRenderer.cpp
    Source/Features/Tracks/Controllers/BankCellInteraction.cpp
    Source/Features/Tracks/Controllers/TrackController.cpp
    Source/Shared/Protocol/ProtocolFrame.cpp
    Source/Host/Plugin/WindowsPluginHost.cpp
    ../Consolidator.Native/External/Transport/ManagedBridge.cpp
    ../Consolidator.Native/External/Dsp/CompressionProcessor.cpp
    ../Consolidator.Native/External/Dsp/CompressorProcessor.cpp
    ../Consolidator.Native/External/Dsp/DetectorProcessor.cpp
    ../Consolidator.Native/External/Dsp/DspAutoGain.cpp
    ../Consolidator.Native/External/Dsp/DspBiquad.cpp
    ../Consolidator.Native/External/Dsp/DspExecution.cpp
    ../Consolidator.Native/External/Dsp/DspGainRamp.cpp
    ../Consolidator.Native/External/Dsp/DspParameterRamp.cpp
    ../Consolidator.Native/External/Dsp/DspParallelStage.cpp
    ../Consolidator.Native/External/Dsp/DspRuntime.cpp
    ../Consolidator.Native/External/Dsp/DspRuntimeGraph.cpp
    ../Consolidator.Native/External/Dsp/DspStateConsumer.cpp
    ../Consolidator.Native/External/Dsp/EqualizerProcessor.cpp
    ../Consolidator.Native/External/Dsp/InputProcessor.cpp
    ../Consolidator.Native/External/Dsp/LevelerProcessor.cpp
    ../Consolidator.Native/External/Dsp/LimiterProcessor.cpp
    ../Consolidator.Native/External/Dsp/OutputProcessor.cpp
    ../Consolidator.Native/External/Dsp/PolishProcessor.cpp
    ../Consolidator.Native/External/Dsp/SaturationProcessor.cpp
    ../Consolidator.Native/External/Dsp/SaturatorProcessor.cpp
    ../Consolidator.Native/External/Dsp/Processors/Air/PolishAirProcessor.cpp
    ../Consolidator.Native/External/Dsp/Processors/Thick/PolishThickProcessor.cpp
    ../Consolidator.Native/External/Dsp/Processors/TransientShaper/TransientShaperProcessor.cpp
    ../Consolidator.Native/External/Dsp/Processors/DeviceCompressor/AdaptiveCompressorGain.cpp
    ../Consolidator.Native/External/Dsp/Processors/DeviceCompressor/DeviceCompressorProcessor.cpp
    ../Consolidator.Native/External/Dsp/Processors/Cla76/Cla76Processor.cpp
    ../Consolidator.Native/External/Dsp/Processors/Cla3a/Cla3aProcessor.cpp
    ../Consolidator.Native/External/Dsp/Processors/Cla2a/Cla2aProcessor.cpp
    ../Consolidator.Native/External/Dsp/Processors/SslChannel/SslChannelProcessor.cpp)
target_link_libraries(Consolidator PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
    juce::juce_recommended_lto_flags)

# Install after copying runtime dependencies into the bundle.
add_custom_command(TARGET Consolidator_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CONSOLIDATOR_MANAGED_RUNTIME_DIR}/Consolidator.Managed.dll"
        "${CONSOLIDATOR_MANAGED_RUNTIME_DIR}/nlopt.dll"
        "$<TARGET_FILE_DIR:Consolidator_VST3>"
    VERBATIM)

add_custom_target(ConsolidatorVST3Install ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CONSOLIDATOR_VST3_INSTALL_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "$<TARGET_FILE_DIR:Consolidator_VST3>/../.."
        "${CONSOLIDATOR_VST3_INSTALL_DIR}/Consolidator.vst3"
    DEPENDS Consolidator_VST3
    COMMENT "Installing Consolidator.vst3 to ${CONSOLIDATOR_VST3_INSTALL_DIR}"
    VERBATIM)
