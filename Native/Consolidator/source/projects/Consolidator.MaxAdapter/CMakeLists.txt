cmake_minimum_required(VERSION 3.10)

add_library(Consolidator.MaxAdapter INTERFACE)

target_include_directories(Consolidator.MaxAdapter
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Consolidator.MaxAdapter INTERFACE
    Consolidator.Shared
    Consolidator.Messaging
)
target_compile_features(Consolidator.MaxAdapter INTERFACE cxx_std_20)
