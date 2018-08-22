# Copyright (c) 2018 the Talvos developers. All rights reserved.
#
# This file is distributed under a three-clause BSD license. For full license
# terms please see the LICENSE file distributed with this source code.

cmake_minimum_required(VERSION 3.8.2)
project(Talvos)

set(TALVOS_VERSION_MAJOR 0)
set(TALVOS_VERSION_MINOR 1)
set(TALVOS_VERSION_PATCH 0)
set(TALVOS_VERSION_SUFFIX "-dev")
configure_file("version.h.in" "version.h")
include_directories(${CMAKE_BINARY_DIR})

include(CheckIncludeFileCXX)

# Enable rpath on OS X
set(CMAKE_MACOSX_RPATH 1)

# Require and enable C++17 support, and disable compiler extensions.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Disable non-secure function warnings for MSVC
if (MSVC)
  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
endif()

include_directories(include)

add_subdirectory(docs)
add_subdirectory(lib)
add_subdirectory(tools)

enable_testing()

# Check for Python
find_package(PythonInterp)
if (PYTHONINTERP_FOUND)
  add_subdirectory(test)
else()
  message(WARNING "Skipping tests (Python required)")
endif()
