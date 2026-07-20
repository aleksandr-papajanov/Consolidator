cmake_minimum_required(VERSION 3.10)

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../min-api)
include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)

include_directories("${C74_INCLUDES}")

set(SOURCE_FILES
    Consolidator.DeviceHost.cpp
)

set("${PROJECT_NAME}_EXTERN_OUTPUT_NAME" "consolidator.devicehost" CACHE STRING "The name to give to the external output file/directory" FORCE)

add_library(${PROJECT_NAME} MODULE ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Consolidator.DeviceHost
        Consolidator.Messaging
        Consolidator.Persistence
        Consolidator.MaxAdapter
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

consolidator_copy_external_to_feature(${PROJECT_NAME} "DeviceHost")

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)
