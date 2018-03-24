# Copyright (c) 2018 the Talvos developers. All rights reserved.
#
# This file is distributed under a three-clause BSD license. For full license
# terms please see the LICENSE file distributed with this source code.

# Export API functions on Windows.
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(DLL_EXPORTS exports.def)
endif()

# Disable non-secure function warnings for MSVC
if (MSVC)
  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
endif()

add_library(talvos-vulkan SHARED
  ${CMAKE_SOURCE_DIR}/include/vulkan/vk_platform.h
  ${CMAKE_SOURCE_DIR}/include/vulkan/vulkan_core.h
  ${DLL_EXPORTS}
  runtime.h
  clears.cpp
  commandbuffers.cpp
  copies.cpp
  descriptorsets.cpp
  device.cpp
  dispatch.cpp
  drawing.cpp
  extensions.cpp
  features.cpp
  fragmentoperations.cpp
  framebuffer.cpp
  fxvertex.cpp
  instance.cpp
  memory.cpp
  pipelines.cpp
  queries.cpp
  rasterization.cpp
  renderpass.cpp
  resources.cpp
  samplers.cpp
  shaders.cpp
  sparseresources.cpp
  synchronization.cpp
)

target_include_directories(talvos-vulkan PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(talvos-vulkan talvos)

install(TARGETS talvos-vulkan DESTINATION lib)
