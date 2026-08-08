cmake_minimum_required(VERSION 3.20)

project(ConsolidatorMax
    VERSION 0.1.0
    DESCRIPTION "Consolidator Min API external"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Min API plumbing.
# C74_MIN_API_DIR defaults to the local copy at Source/Max/MinApi.
if(NOT DEFINED C74_MIN_API_DIR)
    set(C74_MIN_API_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../MinApi")
endif()
include("${C74_MIN_API_DIR}/script/min-pretarget.cmake")

add_library(${PROJECT_NAME} MODULE
    ConsolidatorExternal.h
    ConsolidatorExternal.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${C74_INCLUDES}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."   # root Source/ for Core headers
)

target_link_libraries(${PROJECT_NAME} PRIVATE ConsolidatorCore)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:ConsolidatorCore>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

include("${C74_MIN_API_DIR}/script/min-posttarget.cmake")
