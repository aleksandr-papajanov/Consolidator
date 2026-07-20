cmake_minimum_required(VERSION 3.10)

add_library(Consolidator.Persistence INTERFACE)

target_include_directories(Consolidator.Persistence
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Consolidator.Persistence
    INTERFACE
        Consolidator.Domain
        Consolidator.Shared
)

target_compile_features(Consolidator.Persistence INTERFACE cxx_std_20)
