cmake_minimum_required(VERSION 3.10)

add_library(Consolidator.DeviceHost STATIC
    EqStore.cpp
    GainStore.cpp
    CompressorStore.cpp
    SaturatorStore.cpp
    DeviceHost.cpp
    Workflows/AnalyzerWorkflow.cpp
    Workflows/FitWorkflow.cpp
)

target_include_directories(Consolidator.DeviceHost
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Consolidator.DeviceHost
    PUBLIC
        Consolidator.Domain
        Consolidator.Messaging
)

target_compile_features(Consolidator.DeviceHost PUBLIC cxx_std_20)

if (MSVC)
    set_property(TARGET Consolidator.DeviceHost PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif ()
