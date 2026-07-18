cmake_minimum_required(VERSION 3.10)

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../min-api)
include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)

include_directories(
    "${C74_INCLUDES}"
)

set(SOURCE_FILES
    ${PROJECT_NAME}.cpp
)

set("${PROJECT_NAME}_EXTERN_OUTPUT_NAME" "consolidator.filter" CACHE STRING "The name to give to the external output file/directory" FORCE)

add_library(
    ${PROJECT_NAME}
    MODULE
    ${SOURCE_FILES}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Consolidator.EqCore
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)

if (CONSOLIDATOR_COPY_BUILT_EXTERNALS_TO_MAX)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CONSOLIDATOR_MAX_EXTERNALS_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${CONSOLIDATOR_MAX_EXTERNALS_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    )
endif ()
