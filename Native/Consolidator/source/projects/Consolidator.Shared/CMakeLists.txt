cmake_minimum_required(VERSION 3.10)

add_library(Consolidator.Shared INTERFACE)

target_include_directories(Consolidator.Shared
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
