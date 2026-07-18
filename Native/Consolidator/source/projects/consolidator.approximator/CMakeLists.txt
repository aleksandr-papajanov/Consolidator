cmake_minimum_required(VERSION 3.10)

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../min-api)
include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)

include_directories(
    "${C74_INCLUDES}"
)

find_package(NLopt CONFIG REQUIRED)

set(SOURCE_FILES
    ${PROJECT_NAME}.cpp
    ApproximatorCurveStore.h
    ApproximatorOutputs.h
    EqOptimizer.h
)

set("${PROJECT_NAME}_EXTERN_OUTPUT_NAME" "consolidator.approximator" CACHE STRING "The name to give to the external output file/directory" FORCE)

add_library(
    ${PROJECT_NAME}
    MODULE
    ${SOURCE_FILES}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Consolidator.EqCore
        NLopt::nlopt
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)

consolidator_copy_external_to_feature(${PROJECT_NAME} "Approximator")
