cmake_minimum_required(VERSION 3.10)

add_library(Consolidator.DspCore INTERFACE)

target_include_directories(Consolidator.DspCore
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Consolidator.DspCore
    INTERFACE
        Consolidator.Shared
        Consolidator.Domain
)

target_compile_features(Consolidator.DspCore INTERFACE cxx_std_20)
