cmake_minimum_required(VERSION 3.10)

if (POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif ()

project(Min-Starter)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

get_filename_component(CONSOLIDATOR_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
    ABSOLUTE
)

set(CONSOLIDATOR_MAX_FEATURES_DIR
    "${CONSOLIDATOR_ROOT}/Max/Features"
)

option(CONSOLIDATOR_COPY_BUILT_EXTERNALS_TO_MAX
    "Copy built Max externals into their owning Max feature after build."
    ON
)

function(consolidator_copy_external_to_feature target featureName)
    if (CONSOLIDATOR_COPY_BUILT_EXTERNALS_TO_MAX)
        set(destinationDirectory
            "${CONSOLIDATOR_MAX_FEATURES_DIR}/${featureName}"
        )

        add_custom_command(
            TARGET ${target}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "${destinationDirectory}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${target}>"
                "${destinationDirectory}/$<TARGET_FILE_NAME:${target}>"
            COMMENT "Copying ${target} external to Max feature ${featureName}"
        )
    endif ()
endfunction()

set(C74_LIBRARY_OUTPUT_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/Externals"
)

enable_testing()

string(REGEX REPLACE "(.*)/" "" THIS_PACKAGE_NAME "${CMAKE_CURRENT_SOURCE_DIR}")


if (${CMAKE_GENERATOR} MATCHES "Xcode")
         if (${XCODE_VERSION} VERSION_LESS 10)
              message(STATUS "Xcode 10 or higher is required. Please install from the Mac App Store.")
            return ()
        endif ()
endif ()

# Misc setup and subroutines
include(${CMAKE_CURRENT_SOURCE_DIR}/Source/min-api/script/min-package.cmake)

# Native foundation layers are declared before Max adapters and externals.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Source/Projects/Consolidator.Shared)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Source/Projects/Consolidator.Domain)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Source/Projects/Consolidator.Messaging)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Source/Projects/Consolidator.DspCore)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Source/Projects/Consolidator.Persistence)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Source/Projects/Consolidator.DeviceHost)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Source/Projects/Consolidator.MaxAdapter)

# Add the Lib, if it exists
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/min-lib/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Source/min-lib)
endif ()

# Generate a project for every folder in the "Source/Projects" folder
SUBDIRLIST(PROJECT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Projects)
foreach (project_dir ${PROJECT_DIRS})
    if (project_dir STREQUAL "Consolidator.Shared" OR
        project_dir STREQUAL "Consolidator.Domain" OR
        project_dir STREQUAL "Consolidator.Messaging" OR
        project_dir STREQUAL "Consolidator.DspCore" OR
        project_dir STREQUAL "Consolidator.Persistence" OR
        project_dir STREQUAL "Consolidator.DeviceHost" OR
        project_dir STREQUAL "Consolidator.MaxAdapter")
        continue()
    endif ()

    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Projects/${project_dir}/CMakeLists.txt")
        message("Generating: ${project_dir}")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Source/Projects/${project_dir})
    endif ()
endforeach ()

# Comment the line below if you want automatic cmake regneration enabled
# set(CMAKE_SUPPRESS_REGENERATION true)
