# ============================================================================ #
# Copyright (c) 2024 NVIDIA Corporation & Affiliates.                          #
# All rights reserved.                                                         #
#                                                                              #
# This source code and the accompanying materials are made available under     #
# the terms of the Apache License 2.0 which accompanies this distribution.     #
# ============================================================================ #

set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

# We need 3.28 because of the `EXCLUDE_FROM_ALL` in FetchContent_Declare
cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(CUDAQX
  VERSION 0.0.0
  LANGUAGES C CXX)

set(CUDAQX_ALL_LIBS "qec;solvers")
set(CUDAQX_ENABLE_LIBS "all" CACHE STRING
  "Semicolon-separated list of libs to build (${CUDAQX_ALL_LIBS}), or \"all\".")

# We don't want to handle "all" later, thus expand it here.
if(CUDAQX_ENABLE_LIBS STREQUAL "all" )
  set(CUDAQX_ENABLE_LIBS ${CUDAQX_ALL_LIBS})
endif()

# Sanity check.
foreach(lib ${CUDAQX_ENABLE_LIBS})
  if (NOT "${lib}" IN_LIST CUDAQX_ALL_LIBS)
    message(FATAL_ERROR "${lib} isn't a known library: ${CUDAQX_ALL_LIBS}.")
  endif()
endforeach()

# Project setup
# ==============================================================================

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add our Modules to the path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

# Include custom CUDA-QX modules
include(CUDA-QX)

# Options
# ==============================================================================

option(CUDAQX_INCLUDE_TESTS "Generate build targets for unit tests." ON)
option(CUDAQX_INCLUDE_DOCS "Generate build targets for the docs." ON)
option(CUDAQX_BINDINGS_PYTHON "Generate build targets for python bindings." ON)

# Top-level External Dependencies
# ==============================================================================

# FIXME for now, we only use library mode
set(CUDAQ_LIBRARY_MODE ON)
find_package(CUDAQ REQUIRED)

# Top-level targets
# ==============================================================================

# Helper targets to collect libraries and python modules
add_custom_target(cudaqx-pymodules)

# Top-level testing
# ==============================================================================

if (CUDAQX_INCLUDE_TESTS)
  include(CTest)
  add_custom_target(CUDAQXUnitTests)
  add_custom_target(run_tests
    COMMAND ${CMAKE_COMMAND} -E env
            PYTHONPATH="${CUDAQ_INSTALL_DIR}:${CMAKE_BINARY_DIR}/python"
            ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS CUDAQXUnitTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )

  if (CUDAQX_BINDINGS_PYTHON)
    set(PYTHON_TEST_DIRS "")
    foreach(lib ${CUDAQX_ENABLE_LIBS})
      list(APPEND PYTHON_TEST_DIRS ../libs/${lib}/python/tests)
    endforeach()
    add_custom_target(run_python_tests
      COMMAND ${CMAKE_COMMAND} -E env
              PYTHONPATH="${CUDAQ_INSTALL_DIR}:${CMAKE_BINARY_DIR}/python"
              pytest -v ${PYTHON_TEST_DIRS}
      DEPENDS cudaqx-pymodules
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
  endif()
endif()

# Hooks setup. If the repo contains a custom pre-push hook, attempt to install
# it. If the user has a different one installed, then warn them but do not fail
# configuration.
# ==============================================================================

# Define the directory where your hooks are stored
set(SRC_HOOK_PRE_PUSH "${CMAKE_SOURCE_DIR}/.githooks/pre-push")

if(EXISTS "${SRC_HOOK_PRE_PUSH}")
  # Get the Git hooks directory from the Git configuration
  execute_process(
    COMMAND git config --get core.hooksPath
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_HOOKS_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  # Determine the target hooks directory
  if(GIT_HOOKS_DIR)
    set(TARGET_HOOKS_DIR "${GIT_HOOKS_DIR}")
  else()
    set(TARGET_HOOKS_DIR "${CMAKE_SOURCE_DIR}/.git/hooks")
  endif()
  set(DST_HOOK_PRE_PUSH "${TARGET_HOOKS_DIR}/pre-push")
  
  if(EXISTS "${DST_HOOK_PRE_PUSH}")
    # Compare the contents of the src and dst hook.
    execute_process(
      COMMAND git hash-object "${DST_HOOK_PRE_PUSH}"
      OUTPUT_VARIABLE SHA_DST
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
      COMMAND git hash-object "${SRC_HOOK_PRE_PUSH}"
      OUTPUT_VARIABLE SHA_SRC
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT SHA_SRC STREQUAL SHA_DST)
      message(WARNING
        "You already have a ${DST_HOOK_PRE_PUSH} script installed. "
        "This configuration script will not overwrite it despite the fact that "
        "it is strongly recommended to use ${SRC_HOOK_PRE_PUSH} in your environment."
        "\nProceed with caution!")
    endif()
  else()
    if(EXISTS "${TARGET_HOOKS_DIR}")
      file(COPY "${SRC_HOOK_PRE_PUSH}"
           DESTINATION "${TARGET_HOOKS_DIR}"
           FILE_PERMISSIONS
             OWNER_READ OWNER_WRITE OWNER_EXECUTE
             GROUP_READ GROUP_EXECUTE
             WORLD_READ WORLD_EXECUTE)
      message(STATUS "Git pre-push hook installed to ${TARGET_HOOKS_DIR}")
    else()
      message(WARNING
        "The Git hooks directory does not exist: ${TARGET_HOOKS_DIR}\n"
        "Are you sure this is a Git repository? Hook cannot be installed."
      )
    endif()
  endif()
endif()

# Directory setup
# ==============================================================================

add_subdirectory(libs/core)

if (CUDAQX_INCLUDE_DOCS)
  add_subdirectory(docs)
endif()

foreach(lib ${CUDAQX_ENABLE_LIBS})
  add_subdirectory(libs/${lib})
endforeach()


# Custom targets
# ==============================================================================
set(ZIP_OUTPUT "${CMAKE_BINARY_DIR}/installed_files.zip")

# Custom target to zip up installed files. The resulting zip file can be used to
# overlay CUDA-QX files into a separate (already existing) CUDA-Q install
# directory.
add_custom_target(zip_installed_files
    COMMAND bash -c "sed 's#^${CMAKE_INSTALL_PREFIX}/##' ${CMAKE_BINARY_DIR}/install_manifest.txt | zip -@ '${ZIP_OUTPUT}'"
    WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
    COMMENT "Creating zip archive of installed files"
    VERBATIM
)
