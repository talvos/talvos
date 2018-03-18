// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "vk_platform.h"
#include "vulkan_core.h"

#include <cassert>
#include <memory>
#include <unordered_set>
#include <vector>

#include "talvos/DescriptorSet.h"

namespace talvos
{
class Device;
class DispatchCommand;
class Function;
class Module;
}; // namespace talvos

// TODO: Remove this when all functions have implementations.
#include <cstdlib>
#include <iostream>
#define TALVOS_ABORT_UNIMPLEMENTED                                             \
  std::cerr << "Talvos: Unimplemented Vulkan API '" << __func__ << "'"         \
            << std::endl;                                                      \
  abort()

struct VkBuffer_T
{
  uint64_t Address;
};

struct VkCommandBuffer_T
{
  // Resources that are currently bound to the command buffer.
  VkPipeline Pipeline;
  talvos::DescriptorSetMap DescriptorSetsGraphics;
  talvos::DescriptorSetMap DescriptorSetsCompute;

  // TODO: Move this into libtalvos?
  std::vector<talvos::DispatchCommand *> Commands;
};

struct VkCommandPool_T
{
  std::unordered_set<VkCommandBuffer> Pool;
};

struct VkDescriptorPool_T
{
  std::unordered_set<VkDescriptorSet> Pool;
};

struct VkDescriptorSet_T
{
  talvos::DescriptorSet DescriptorSet;
};

struct VkDevice_T
{
  talvos::Device *Device;
};

struct VkDeviceMemory_T
{
  uint64_t Address;
};

struct VkInstance_T
{
};

struct VkPipeline_T
{
  const talvos::Module *Module;
  const talvos::Function *Function;
};

struct VkQueue_T
{
  VkDevice Device;
};

struct VkShaderModule_T
{
  std::unique_ptr<talvos::Module> Module;
};
