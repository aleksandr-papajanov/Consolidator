cmake_minimum_required(VERSION 3.10)

add_library(Consolidator.Shared STATIC
    DSP/Dsp.cpp
)

target_include_directories(Consolidator.Shared
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(Consolidator.Shared PUBLIC cxx_std_20)
