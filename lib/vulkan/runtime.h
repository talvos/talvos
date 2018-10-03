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

#include "talvos/Commands.h"
#include "talvos/PipelineContext.h"

namespace talvos
{
class ComputePipeline;
class Device;
class Framebuffer;
class Function;
class GraphicsPipeline;
class Image;
class ImageView;
class Module;
class Queue;
class RenderPass;
class RenderPassInstance;
class Sampler;
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
  VkBufferUsageFlags UsageFlags;
};

struct VkBufferView_T
{
  talvos::Image *Image;
  talvos::ImageView *ImageView;
  uint64_t ObjectAddress;
};

struct VkCommandBuffer_T
{
  // Pipeline state that is currently bound to the command buffer.
  talvos::PipelineContext PipelineContext;

  // The current render pass instance.
  std::shared_ptr<talvos::RenderPassInstance> RenderPassInstance;

  // Parameters for indexed draw commands.
  uint64_t IndexBufferAddress;
  VkIndexType IndexType;

  // TODO: Move this into libtalvos?
  std::vector<talvos::Command *> Commands;
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
  VkDescriptorSetLayout Layout;
  talvos::DescriptorSet DescriptorSet;

  /// Map from Binding/ArrayElement pair to address of SampledImage object.
  std::map<std::pair<uint32_t, uint32_t>, uint64_t> CombinedImageSamplers;
};

struct VkDescriptorSetLayout_T
{
  /// Map from binding number to descriptor count.
  std::map<uint32_t, uint32_t> BindingCounts;

  /// Map from binding number to descriptor type.
  std::map<uint32_t, VkDescriptorType> BindingTypes;

  /// Map from binding number to list of immutable sampler addresses.
  std::map<uint32_t, std::vector<VkSampler>> ImmutableSamplers;
};

struct VkDescriptorUpdateTemplate_T
{
  uint32_t EntryCount;
  VkDescriptorUpdateTemplateEntry *Entries;
};

struct VkDevice_T
{
  talvos::Device *Device;
  talvos::Queue *Queue;
};

struct VkDeviceMemory_T
{
  uint64_t Address;
  uint64_t NumBytes;
};

struct VkEvent_T
{
  bool Signaled;
};

struct VkFence_T
{
  bool Signaled;
};

struct VkFramebuffer_T
{
  talvos::Framebuffer *Framebuffer;
};

struct VkImage_T
{
  talvos::Image *Image;
};

struct VkImageView_T
{
  talvos::ImageView *ImageView;
  uint64_t ObjectAddress;
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
  const talvos::ComputePipeline *ComputePipeline = nullptr;
  const talvos::GraphicsPipeline *GraphicsPipeline = nullptr;
};

struct VkPipelineCache_T
{
};

struct VkPipelineLayout_T
{
};

struct VkQueue_T
{
  talvos::Queue *Queue;
};

struct VkRenderPass_T
{
  talvos::RenderPass *RenderPass;
};

struct VkSampler_T
{
  talvos::Sampler *Sampler;
  uint64_t ObjectAddress;
};

struct VkSemaphore_T
{
  bool Signaled;
};

struct VkShaderModule_T
{
  std::shared_ptr<talvos::Module> Module;
};

extern const uint32_t NumDeviceExtensions;
extern const VkExtensionProperties DeviceExtensions[];
