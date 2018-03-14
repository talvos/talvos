# Copyright (c) 2018 the Talvos developers. All rights reserved.
#
# This file is distributed under a three-clause BSD license. For full license
# terms please see the LICENSE file distributed with this source code.

add_library(talvos-vulkan SHARED
  runtime.h
  vk_platform.h
  vulkan_core.h
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

install(TARGETS talvos-vulkan DESTINATION lib)
