# Copyright 2018 The Min-DevKit Authors. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.

cmake_minimum_required(VERSION 3.10)

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../min-api)
include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)


#############################################################
# MAX EXTERNAL
#############################################################


include_directories( 
	"${C74_INCLUDES}"
)

set(SOURCE_FILES
    ${PROJECT_NAME}.cpp
    AnalyzerFrameBuffer.h
    AnalyzerStatistics.h
    AnalyzerCurveBatch.h
    AnalyzerFilterVisuals.h
    AnalyzerSpectrumEngine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ThirdParty/KissFFT/kiss_fft.c
)

set("${PROJECT_NAME}_EXTERN_OUTPUT_NAME" "consolidator.analyzer" CACHE STRING "The name to give to the external output file/directory" FORCE)

add_library(
    ${PROJECT_NAME}
    MODULE
    ${SOURCE_FILES}
 )

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Consolidator.EqCore
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../ThirdParty/KissFFT
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4267)
endif ()

if (CONSOLIDATOR_COPY_BUILT_EXTERNALS_TO_MAX)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${CONSOLIDATOR_MAX_EXTERNALS_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    )
endif ()


target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)
