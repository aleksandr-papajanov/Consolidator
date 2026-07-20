cmake_minimum_required(VERSION 3.10)

add_library(Consolidator.Domain INTERFACE)

target_include_directories(Consolidator.Domain
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Consolidator.Domain
    INTERFACE
        Consolidator.Shared
)

target_compile_features(Consolidator.Domain INTERFACE cxx_std_20)
