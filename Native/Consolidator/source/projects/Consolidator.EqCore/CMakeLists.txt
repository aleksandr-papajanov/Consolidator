cmake_minimum_required(VERSION 3.10)

add_library(Consolidator.EqCore INTERFACE)

target_include_directories(Consolidator.EqCore
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
