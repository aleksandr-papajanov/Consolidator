cmake_minimum_required(VERSION 3.10)

add_library(Consolidator.Messaging INTERFACE)

target_include_directories(Consolidator.Messaging
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Consolidator.Messaging
    INTERFACE
        Consolidator.Domain
)

target_compile_features(Consolidator.Messaging INTERFACE cxx_std_20)
