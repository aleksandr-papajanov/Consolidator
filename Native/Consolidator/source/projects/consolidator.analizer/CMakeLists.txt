# Copyright 2018 The Min-DevKit Authors. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.

cmake_minimum_required(VERSION 3.5)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../ThirdParty/kissfft/kiss_fft.c
)

add_library(
    ${PROJECT_NAME}
    MODULE
    ${SOURCE_FILES}
 )

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../ThirdParty/kissfft
)

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "${CONSOLIDATOR_MAX_EXTERNALS_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
)


target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)

# Copy the built external to the Max package directory

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CONSOLIDATOR_MAX_EXTERNALS_DIR}"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "${CONSOLIDATOR_MAX_EXTERNALS_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"

    COMMENT "Copying ${EXTERNAL_NAME} external to Max/externals"
)