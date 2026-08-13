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
    MinApiImplementation.cpp
    ../Protocol/MaxProtocolAdapter.h
    ../Protocol/MaxProtocolAdapter.cpp
    ../Protocol/AtomPathCodec.h
    ../Protocol/AtomPathCodec.cpp
    ../Protocol/AtomValueCodec.h
    ../Protocol/AtomValueCodec.cpp
    ../Protocol/AtomCommandDecoder.h
    ../Protocol/AtomCommandDecoder.cpp
    ../Protocol/AtomResponseEncoder.h
    ../Protocol/AtomResponseEncoder.cpp
    ../Protocol/WireIdCodec.h
    ../Protocol/WireIdCodec.cpp
    ../Protocol/BankIdCodec.h
    ../Protocol/BankIdCodec.cpp
)

option(CONSOLIDATOR_COPY_BUILT_EXTERNALS_TO_MAX
    "Copy native Max artifacts to the working Consolidator.Max domain" OFF)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${C74_INCLUDES}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."   # root Source/ for Core headers
    "${CMAKE_CURRENT_SOURCE_DIR}/../Protocol"
)

target_link_libraries(${PROJECT_NAME} PRIVATE ConsolidatorCore)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:ConsolidatorCore>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

if(CONSOLIDATOR_COPY_BUILT_EXTERNALS_TO_MAX)
    set(CONSOLIDATOR_MAX_EXTERNALS_DIR
        "${CMAKE_SOURCE_DIR}/../Consolidator.Max/externals")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CONSOLIDATOR_MAX_EXTERNALS_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${CONSOLIDATOR_MAX_EXTERNALS_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:ConsolidatorCore>"
            "${CONSOLIDATOR_MAX_EXTERNALS_DIR}"
        COMMENT "Copying Consolidator Max artifacts to Consolidator.Max/externals"
    )
endif()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

include("${C74_MIN_API_DIR}/script/min-posttarget.cmake")

add_executable(MaxProtocolTests
    ../../../Tests/Component/Max/MaxProtocolTests.cpp
    ../Protocol/MaxProtocolAdapter.cpp
    ../Protocol/AtomCommandDecoder.cpp
    ../Protocol/AtomPathCodec.cpp
    ../Protocol/AtomValueCodec.cpp
    ../Protocol/AtomResponseEncoder.cpp
    ../Protocol/WireIdCodec.cpp
    ../Protocol/BankIdCodec.cpp
    ../../../Tests/Support/MaxApiTestStubs.cpp
)

target_include_directories(MaxProtocolTests PRIVATE
    "${C74_INCLUDES}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../Tests"
)

target_link_libraries(MaxProtocolTests PRIVATE ConsolidatorCore)
target_compile_features(MaxProtocolTests PRIVATE cxx_std_20)

add_test(NAME MaxProtocolTests COMMAND MaxProtocolTests)
set_tests_properties(MaxProtocolTests PROPERTIES LABELS component)
