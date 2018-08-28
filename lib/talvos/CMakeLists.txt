# Copyright (c) 2018 the Talvos developers. All rights reserved.
#
# This file is distributed under a three-clause BSD license. For full license
# terms please see the LICENSE file distributed with this source code.

include(CheckIncludeFiles)
include(CheckLibraryExists)
include(FindThreads)

# Check for SPIRV headers
set(SPIRV_INCLUDE_DIR "" CACHE STRING "Directory containing SPIR-V headers")
set(CMAKE_REQUIRED_INCLUDES "${SPIRV_INCLUDE_DIR}")
unset(SPIRV_HEADER_FOUND CACHE)
check_include_file_cxx("spirv/unified1/spirv.h" SPIRV_HEADER_FOUND)
if (NOT SPIRV_HEADER_FOUND)
  message(FATAL_ERROR "SPIR-V header required, try setting SPIRV_INCLUDE_DIR.")
endif()

# Check for SPIRV-Tools header
set(SPIRV_TOOLS_INCLUDE_DIR "" CACHE STRING
    "Directory containing SPIRV-Tools headers")
set(CMAKE_REQUIRED_INCLUDES "${SPIRV_TOOLS_INCLUDE_DIR}")
unset(SPIRV_TOOLS_HEADER_FOUND CACHE)
check_include_file_cxx("spirv-tools/libspirv.hpp" SPIRV_TOOLS_HEADER_FOUND)
if (NOT SPIRV_TOOLS_HEADER_FOUND)
  message(FATAL_ERROR
          "SPIRV-Tools header required, try setting SPIRV_TOOLS_INCLUDE_DIR.")
endif()

# Check for SPIRV-Tools library
set(SPIRV_TOOLS_LIBRARY_DIR "" CACHE STRING
    "Directory containing SPIRV-Tools libraries")
find_library(SPIRV_TOOLS_LIB SPIRV-Tools PATHS "${SPIRV_TOOLS_LIBRARY_DIR}")
message(STATUS "SPIRV-Tools library: ${SPIRV_TOOLS_LIB}")
if (NOT SPIRV_TOOLS_LIB)
  message(FATAL_ERROR
          "SPIRV-Tools library required, try setting SPIRV_TOOLS_LIBRARY_DIR.")
endif()

# Check for GNU readline library
if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  check_include_files("stdio.h;readline/readline.h" HAVE_READLINE_H)
  check_include_files("stdio.h;readline/history.h" HAVE_HISTORY_H)
  check_library_exists(readline readline
                       "${READLINE_DIR}/lib" HAVE_READLINE_LIB)
  check_library_exists(readline add_history
                       "${READLINE_DIR}/lib" HAVE_HISTORY_LIB)
  if (HAVE_READLINE_H AND HAVE_HISTORY_H AND
      HAVE_READLINE_LIB AND HAVE_HISTORY_LIB)
    set(HAVE_READLINE 1)
    list(APPEND TALVOS_OPTIONAL_LIBS readline)
  else()
    set(HAVE_READLINE 0)
    message(WARNING "GNU readline library not found.\n"
                    "The interactive debugger will not have a command history.")
  endif()
else()
  set(HAVE_READLINE 0)
endif()

# Disable exceptions for libtalvos
if (NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
endif()

# Generate config.h
configure_file("config.h.in" "config.h")

set(TALVOS_HEADERS
    ${CMAKE_SOURCE_DIR}/include/talvos/Block.h
    ${CMAKE_SOURCE_DIR}/include/talvos/Commands.h
    ${CMAKE_SOURCE_DIR}/include/talvos/ComputePipeline.h
    ${CMAKE_SOURCE_DIR}/include/talvos/DescriptorSet.h
    ${CMAKE_SOURCE_DIR}/include/talvos/Device.h
    ${CMAKE_SOURCE_DIR}/include/talvos/Dim3.h
    ${CMAKE_SOURCE_DIR}/include/talvos/EntryPoint.h
    ${CMAKE_SOURCE_DIR}/include/talvos/Function.h
    ${CMAKE_SOURCE_DIR}/include/talvos/GraphicsPipeline.h
    ${CMAKE_SOURCE_DIR}/include/talvos/Image.h
    ${CMAKE_SOURCE_DIR}/include/talvos/Instruction.h
    ${CMAKE_SOURCE_DIR}/include/talvos/Invocation.h
    ${CMAKE_SOURCE_DIR}/include/talvos/Memory.h
    ${CMAKE_SOURCE_DIR}/include/talvos/Module.h
    ${CMAKE_SOURCE_DIR}/include/talvos/Object.h
    ${CMAKE_SOURCE_DIR}/include/talvos/PipelineStage.h
    ${CMAKE_SOURCE_DIR}/include/talvos/Plugin.h
    ${CMAKE_SOURCE_DIR}/include/talvos/Queue.h
    ${CMAKE_SOURCE_DIR}/include/talvos/RenderPass.h
    ${CMAKE_SOURCE_DIR}/include/talvos/Type.h
    ${CMAKE_SOURCE_DIR}/include/talvos/Variable.h
    ${CMAKE_SOURCE_DIR}/include/talvos/Workgroup.h)
set(TALVOS_SOURCES
    Block.cpp
    Commands.cpp
    ComputePipeline.cpp
    Device.cpp
    Dim3.cpp
    Function.cpp
    GraphicsPipeline.cpp
    Image.cpp
    Instruction.cpp
    Invocation.cpp
    Memory.cpp
    Module.cpp
    Object.cpp
    PipelineExecutor.cpp
    PipelineExecutor.h
    PipelineStage.cpp
    Queue.cpp
    RenderPass.cpp
    Type.cpp
    Variable.cpp
    Workgroup.cpp
    Utils.cpp
    Utils.h)

if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(CORE_LIB_TYPE SHARED)
endif()

add_library(talvos ${CORE_LIB_TYPE} ${TALVOS_SOURCES} ${TALVOS_HEADERS})
target_include_directories(talvos PRIVATE "${SPIRV_INCLUDE_DIR}")
target_include_directories(talvos PRIVATE "${SPIRV_TOOLS_INCLUDE_DIR}")
target_include_directories(talvos PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(talvos "${SPIRV_TOOLS_LIB}")
target_link_libraries(talvos "${CMAKE_DL_LIBS}")
target_link_libraries(talvos "${CMAKE_THREAD_LIBS_INIT}")
target_link_libraries(talvos "${TALVOS_OPTIONAL_LIBS}")

install(TARGETS talvos DESTINATION lib)
install(FILES ${TALVOS_HEADERS} DESTINATION include/talvos)
