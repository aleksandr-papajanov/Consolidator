cmake_minimum_required(VERSION 3.22)

project(ConsolidatorModelPlugins VERSION 0.3.0)

include(FetchContent)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(juce)

function(configure_model_plugin target)
    juce_generate_juce_header(${target})
    target_include_directories(${target} PRIVATE
        ../Consolidator.Native/External/Dsp)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_compile_definitions(${target} PRIVATE
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)
    target_link_libraries(${target} PRIVATE
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
        juce::juce_recommended_lto_flags)
endfunction()

juce_add_plugin(ConsolidatorThickModel
    COMPANY_NAME "Consolidator"
    BUNDLE_ID "com.consolidator.thick-model"
    PLUGIN_MANUFACTURER_CODE Cnsl
    PLUGIN_CODE ThMd
    FORMATS VST3
    PRODUCT_NAME "Consolidator Thick Model"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)
target_sources(ConsolidatorThickModel PRIVATE
    Source/ThickAirModelPluginProcessor.cpp
    ../Consolidator.Native/External/Dsp/DspBiquad.cpp
    ../Consolidator.Native/External/Dsp/DspParameterRamp.cpp
    ../Consolidator.Native/External/Dsp/Processors/Thick/PolishThickProcessor.cpp)
target_compile_definitions(ConsolidatorThickModel PRIVATE
    CONSOLIDATOR_THICK_MODEL=1)
configure_model_plugin(ConsolidatorThickModel)

juce_add_plugin(ConsolidatorAirModel
    COMPANY_NAME "Consolidator"
    BUNDLE_ID "com.consolidator.air-model"
    PLUGIN_MANUFACTURER_CODE Cnsl
    PLUGIN_CODE AiMd
    FORMATS VST3
    PRODUCT_NAME "Consolidator Air Model"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)
target_sources(ConsolidatorAirModel PRIVATE
    Source/ThickAirModelPluginProcessor.cpp
    ../Consolidator.Native/External/Dsp/DspParameterRamp.cpp
    ../Consolidator.Native/External/Dsp/Processors/Air/PolishAirProcessor.cpp)
target_compile_definitions(ConsolidatorAirModel PRIVATE
    CONSOLIDATOR_AIR_MODEL=1)
configure_model_plugin(ConsolidatorAirModel)

juce_add_plugin(ConsolidatorTransientShaperModel
    COMPANY_NAME "Consolidator"
    BUNDLE_ID "com.consolidator.transient-shaper-model"
    PLUGIN_MANUFACTURER_CODE Cnsl
    PLUGIN_CODE TrMd
    FORMATS VST3
    PRODUCT_NAME "Consolidator Transient Shaper Model"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)
target_sources(ConsolidatorTransientShaperModel PRIVATE
    Source/TransientShaperPluginProcessor.cpp
    ../Consolidator.Native/External/Dsp/Processors/TransientShaper/TransientShaperProcessor.cpp
    ../Consolidator.Native/External/Dsp/DspParameterRamp.cpp)
configure_model_plugin(ConsolidatorTransientShaperModel)


juce_add_plugin(ConsolidatorCla76Model
    COMPANY_NAME "Consolidator"
    BUNDLE_ID "com.consolidator.cla76-bluey-model"
    PLUGIN_MANUFACTURER_CODE Cnsl
    PLUGIN_CODE Cl76
    FORMATS VST3
    PRODUCT_NAME "Consolidator CLA-76 Bluey Model"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)
target_sources(ConsolidatorCla76Model PRIVATE
    Source/Cla76PluginProcessor.cpp
    ../Consolidator.Native/External/Dsp/Processors/Cla76/Cla76Processor.cpp
    ../Consolidator.Native/External/Dsp/DspParameterRamp.cpp)
configure_model_plugin(ConsolidatorCla76Model)

juce_add_plugin(ConsolidatorCla3aModel
    COMPANY_NAME "Consolidator"
    BUNDLE_ID "com.consolidator.cla3a-model"
    PLUGIN_MANUFACTURER_CODE Cnsl
    PLUGIN_CODE Cl3a
    FORMATS VST3
    PRODUCT_NAME "Consolidator CLA-3A Model"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)
target_sources(ConsolidatorCla3aModel PRIVATE
    Source/Cla3aPluginProcessor.cpp
    ../Consolidator.Native/External/Dsp/Processors/Cla3a/Cla3aProcessor.cpp
    ../Consolidator.Native/External/Dsp/DspBiquad.cpp
    ../Consolidator.Native/External/Dsp/DspParameterRamp.cpp)
configure_model_plugin(ConsolidatorCla3aModel)
