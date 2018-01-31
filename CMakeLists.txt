# Copyright (c) 2018 the Talvos developers. All rights reserved.
#
# This file is distributed under a three-clause BSD license. For full license
# terms please see the LICENSE file distributed with this source code.

cmake_minimum_required(VERSION 2.8.12)
project(Talvos)

include(CheckCXXCompilerFlag)
include(CheckIncludeFileCXX)

# Enable rpath on OS X
set(CMAKE_MACOSX_RPATH 1)

if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  check_cxx_compiler_flag("-std=c++14" HAS_CXX14)
  if (NOT HAS_CXX14)
    message(FATAL_ERROR "A compiler with support for C++14 is required")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")
endif()

include_directories(include)

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
