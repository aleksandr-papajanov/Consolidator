cmake_minimum_required(VERSION 3.22)

project(EmulationLab VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(juce)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(json)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(NLOPT_CXX ON CACHE BOOL "" FORCE)
set(NLOPT_FORTRAN OFF CACHE BOOL "" FORCE)
set(NLOPT_PYTHON OFF CACHE BOOL "" FORCE)
set(NLOPT_OCTAVE OFF CACHE BOOL "" FORCE)
set(NLOPT_MATLAB OFF CACHE BOOL "" FORCE)
set(NLOPT_GUILE OFF CACHE BOOL "" FORCE)
set(NLOPT_JAVA OFF CACHE BOOL "" FORCE)
set(NLOPT_SWIG OFF CACHE BOOL "" FORCE)
set(NLOPT_TESTS OFF CACHE BOOL "" FORCE)
set(NLOPT_LUKSAN OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    nlopt
    GIT_REPOSITORY https://github.com/stevengj/nlopt.git
    GIT_TAG v2.11.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nlopt)

juce_add_console_app(EmulationLab
    COMPANY_NAME "Consolidator"
    PRODUCT_NAME "Emulation Lab")

juce_generate_juce_header(EmulationLab)

target_sources(EmulationLab PRIVATE
    Source/AudioFileRender.cpp
    Source/Comparison.cpp
    Source/Capture.cpp
    Source/Dataset.cpp
    Source/Diagnostics.cpp
    Source/ExperimentManifest.cpp
    Source/LevelSweepProbe.cpp
    Source/MultitoneProbe.cpp
    Source/Main.cpp
    Source/RunMetadata.cpp
    Source/SteppedSineProbe.cpp
    Source/StereoInteractionProbe.cpp
    Source/TransientBurstProbe.cpp
    Source/TransientEnvelopeAnalysis.cpp
    Source/Host/Vst3Host.cpp
    Source/Optimization/EmulationOptimizer.cpp
    Source/Optimization/PluginParameterProblem.cpp)

target_include_directories(EmulationLab PRIVATE
    Source
    Source/Host)
target_compile_features(EmulationLab PRIVATE cxx_std_20)
target_compile_definitions(EmulationLab PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
target_link_libraries(EmulationLab PRIVATE
    nlopt
    nlohmann_json::nlohmann_json
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
