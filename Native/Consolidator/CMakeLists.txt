cmake_minimum_required(VERSION 3.10)
project(Min-Starter)

get_filename_component(CONSOLIDATOR_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
    ABSOLUTE
)

set(CONSOLIDATOR_MAX_EXTERNALS_DIR
    "${CONSOLIDATOR_ROOT}/Max/Externals"
)

option(CONSOLIDATOR_COPY_BUILT_EXTERNALS_TO_MAX
    "Copy built Max externals into Max/Externals after build."
    ON
)

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

# Add the Lib, if it exists
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/min-lib/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Source/min-lib)
endif ()

# Generate a project for every folder in the "Source/Projects" folder
SUBDIRLIST(PROJECT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Projects)
foreach (project_dir ${PROJECT_DIRS})
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Projects/${project_dir}/CMakeLists.txt")
        message("Generating: ${project_dir}")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Source/Projects/${project_dir})
    endif ()
endforeach ()

# Comment the line below if you want automatic cmake regneration enabled
# set(CMAKE_SUPPRESS_REGENERATION true)
