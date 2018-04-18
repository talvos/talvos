// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "vulkan/vk_platform.h"
#include "vulkan/vulkan_core.h"

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
class Pipeline;
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
  VkDeviceSize NumBytes;
  uint64_t Address;
};

struct VkBufferView_T
{
  VkFormat Format;
  VkDeviceSize NumBytes;
  uint64_t Address;
};

struct VkCommandBuffer_T
{
  // Resources that are currently bound to the command buffer.
  VkPipeline PipelineGraphics;
  VkPipeline PipelineCompute;
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

struct VkDescriptorSetLayout_T
{
};

struct VkDevice_T
{
  talvos::Device *Device;
};

struct VkDeviceMemory_T
{
  uint64_t Address;
};

struct VkEvent_T
{
  bool Signaled;
};

struct VkFence_T
{
  bool Signaled;
};

struct VkImage_T
{
  VkImageType Type;
  VkFormat Format;
  VkExtent3D Extent;
  uint32_t MipLevels;
  uint32_t ArrayLayers;
  //VkDeviceSize NumBytes;
  uint64_t Address;
};

struct VkImageView_T
{
  VkImage Image;
  VkImageViewType Type;
  VkFormat Format;
};

struct VkInstance_T
{
  VkPhysicalDevice Device;
};

struct VkPhysicalDevice_T
{
  VkPhysicalDeviceFeatures Features;
};

struct VkPipeline_T
{
  const talvos::Pipeline *Pipeline;
};

struct VkPipelineLayout_T
{
};

struct VkQueue_T
{
  VkDevice Device;
};

struct VkSemaphore_T
{
  bool Signaled;
};

struct VkShaderModule_T
{
  std::unique_ptr<talvos::Module> Module;
};
