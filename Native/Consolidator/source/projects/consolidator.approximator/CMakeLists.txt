cmake_minimum_required(VERSION 3.5)

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../min-api)
include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)

include_directories(
    "${C74_INCLUDES}"
)

set(SOURCE_FILES
    ${PROJECT_NAME}.cpp
)

add_library(
    ${PROJECT_NAME}
    MODULE
    ${SOURCE_FILES}
    EqParams.h
    EqModel.h
)


target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CONSOLIDATOR_MAX_EXTERNALS_DIR}"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "${CONSOLIDATOR_MAX_EXTERNALS_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"

    COMMENT "Copying ${PROJECT_NAME} external and nlopt.dll to Max/externals"
)