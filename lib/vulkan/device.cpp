// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

#include <cstring>

#include "talvos/Device.h"

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
    VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkDevice *pDevice)
{
  // Check extensions are supported.
  for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++)
  {
    // TODO: Check whether we actually can support the extension.
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  // Check features are supported.
  if (pCreateInfo->pEnabledFeatures)
  {
    for (uint32_t i = 0;
         i < sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32); i++)
    {
      if (((VkBool32 *)pCreateInfo->pEnabledFeatures)[i] &&
          !((VkBool32 *)&physicalDevice->Features)[i])
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }
  }

  *pDevice = new VkDevice_T;
  (*pDevice)->Device = new talvos::Device;
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator)
{
  delete device->Device;
  delete device;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDeviceGroups(
    VkInstance instance, uint32_t *pPhysicalDeviceGroupCount,
    VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties)
{
  if (!pPhysicalDeviceGroupProperties)
  {
    *pPhysicalDeviceGroupCount = 1;
    return VK_SUCCESS;
  }

  if (*pPhysicalDeviceGroupCount < 1)
    return VK_INCOMPLETE;

  *pPhysicalDeviceGroupCount = 1;
  pPhysicalDeviceGroupProperties->physicalDeviceCount = 1;
  pPhysicalDeviceGroupProperties->subsetAllocation = VK_FALSE;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDeviceGroupsKHR(
    VkInstance instance, uint32_t *pPhysicalDeviceGroupCount,
    VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount,
                           VkPhysicalDevice *pPhysicalDevices)
{
  if (pPhysicalDevices && *pPhysicalDeviceCount < 1)
    return VK_INCOMPLETE;

  *pPhysicalDeviceCount = 1;
  if (pPhysicalDevices)
    *pPhysicalDevices = instance->Device;

  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(VkDevice device,
                                            uint32_t queueFamilyIndex,
                                            uint32_t queueIndex,
                                            VkQueue *pQueue)
{
  *pQueue = new VkQueue_T;
  (*pQueue)->Device = device;
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue2(
    VkDevice device, const VkDeviceQueueInfo2 *pQueueInfo, VkQueue *pQueue)
{
  *pQueue = new VkQueue_T;
  (*pQueue)->Device = device;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties)
{
  pProperties->apiVersion = VK_API_VERSION_1_1;
  pProperties->driverVersion = 0; // TODO: Encode Talvos version somehow.
  pProperties->vendorID = 0;      // TODO: Register a vendor ID.
  pProperties->deviceID = 0;      // TODO: Something meaningful.
  pProperties->deviceType = VK_PHYSICAL_DEVICE_TYPE_OTHER;
  memset(pProperties->pipelineCacheUUID, 0,
         sizeof(pProperties->pipelineCacheUUID));
  strcpy(pProperties->deviceName, "Talvos");

  pProperties->limits.maxImageDimension1D = 4096;
  pProperties->limits.maxImageDimension2D = 4096;
  pProperties->limits.maxImageDimension3D = 256;
  pProperties->limits.maxImageDimensionCube = 4096;
  pProperties->limits.maxImageArrayLayers = 256;
  pProperties->limits.maxTexelBufferElements = 65536;
  pProperties->limits.maxUniformBufferRange = 16384;
  pProperties->limits.maxStorageBufferRange = (1 << 27);
  pProperties->limits.maxPushConstantsSize = 128;
  pProperties->limits.maxMemoryAllocationCount = 4096;
  pProperties->limits.maxSamplerAllocationCount = 4000;
  pProperties->limits.bufferImageGranularity = 131072;
  pProperties->limits.sparseAddressSpaceSize = 0;
  pProperties->limits.maxBoundDescriptorSets = 4;
  pProperties->limits.maxPerStageDescriptorSamplers = 16;
  pProperties->limits.maxPerStageDescriptorUniformBuffers = 12;
  pProperties->limits.maxPerStageDescriptorStorageBuffers = 4;
  pProperties->limits.maxPerStageDescriptorSampledImages = 16;
  pProperties->limits.maxPerStageDescriptorStorageImages = 4;
  pProperties->limits.maxPerStageDescriptorInputAttachments = 4;
  pProperties->limits.maxPerStageResources = 128;
  pProperties->limits.maxDescriptorSetSamplers = 96;
  pProperties->limits.maxDescriptorSetUniformBuffers = 72;
  pProperties->limits.maxDescriptorSetUniformBuffersDynamic = 8;
  pProperties->limits.maxDescriptorSetStorageBuffers = 24;
  pProperties->limits.maxDescriptorSetStorageBuffersDynamic = 4;
  pProperties->limits.maxDescriptorSetSampledImages = 96;
  pProperties->limits.maxDescriptorSetStorageImages = 24;
  pProperties->limits.maxDescriptorSetInputAttachments = 4;
  pProperties->limits.maxVertexInputAttributes = 16;
  pProperties->limits.maxVertexInputBindings = 16;
  pProperties->limits.maxVertexInputAttributeOffset = 2047;
  pProperties->limits.maxVertexInputBindingStride = 2048;
  pProperties->limits.maxVertexOutputComponents = 64;
  pProperties->limits.maxTessellationGenerationLevel = 0;
  pProperties->limits.maxTessellationPatchSize = 0;
  pProperties->limits.maxTessellationControlPerVertexInputComponents = 0;
  pProperties->limits.maxTessellationControlPerVertexOutputComponents = 0;
  pProperties->limits.maxTessellationControlPerPatchOutputComponents = 0;
  pProperties->limits.maxTessellationControlTotalOutputComponents = 0;
  pProperties->limits.maxTessellationEvaluationInputComponents = 0;
  pProperties->limits.maxTessellationEvaluationOutputComponents = 0;
  pProperties->limits.maxGeometryShaderInvocations = 0;
  pProperties->limits.maxGeometryInputComponents = 0;
  pProperties->limits.maxGeometryOutputComponents = 0;
  pProperties->limits.maxGeometryOutputVertices = 0;
  pProperties->limits.maxGeometryTotalOutputComponents = 0;
  pProperties->limits.maxFragmentInputComponents = 64;
  pProperties->limits.maxFragmentOutputAttachments = 4;
  pProperties->limits.maxFragmentDualSrcAttachments = 0;
  pProperties->limits.maxFragmentCombinedOutputResources = 4;
  pProperties->limits.maxComputeSharedMemorySize = 16384;
  pProperties->limits.maxComputeWorkGroupCount[0] = 65536;
  pProperties->limits.maxComputeWorkGroupCount[1] = 65536;
  pProperties->limits.maxComputeWorkGroupCount[2] = 65536;
  pProperties->limits.maxComputeWorkGroupInvocations = 128;
  pProperties->limits.maxComputeWorkGroupSize[0] = 128;
  pProperties->limits.maxComputeWorkGroupSize[1] = 128;
  pProperties->limits.maxComputeWorkGroupSize[2] = 64;
  pProperties->limits.subPixelPrecisionBits = 4;
  pProperties->limits.subTexelPrecisionBits = 4;
  pProperties->limits.mipmapPrecisionBits = 4;
  pProperties->limits.maxDrawIndexedIndexValue = (1 << 24) - 1;
  pProperties->limits.maxDrawIndirectCount = 1;
  pProperties->limits.maxSamplerLodBias = 2;
  pProperties->limits.maxSamplerAnisotropy = 1;
  pProperties->limits.maxViewports = 1;
  pProperties->limits.maxViewportDimensions[0] = 4096;
  pProperties->limits.maxViewportDimensions[1] = 4096;
  pProperties->limits.viewportBoundsRange[0] = -8192;
  pProperties->limits.viewportBoundsRange[1] = 8191;
  pProperties->limits.viewportSubPixelBits = 0;
  pProperties->limits.minMemoryMapAlignment = 64;
  pProperties->limits.minTexelBufferOffsetAlignment = 256;
  pProperties->limits.minUniformBufferOffsetAlignment = 256;
  pProperties->limits.minStorageBufferOffsetAlignment = 256;
  pProperties->limits.minTexelOffset = -8;
  pProperties->limits.maxTexelOffset = -7;
  pProperties->limits.minTexelGatherOffset = 0;
  pProperties->limits.maxTexelGatherOffset = 0;
  pProperties->limits.minInterpolationOffset = 0.0;
  pProperties->limits.maxInterpolationOffset = 0.0;
  pProperties->limits.subPixelInterpolationOffsetBits = 0;
  pProperties->limits.maxFramebufferWidth = 4096;
  pProperties->limits.maxFramebufferHeight = 4096;
  pProperties->limits.maxFramebufferLayers = 256;
  pProperties->limits.framebufferColorSampleCounts =
      VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_4_BIT;
  pProperties->limits.framebufferDepthSampleCounts =
      VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_4_BIT;
  pProperties->limits.framebufferStencilSampleCounts =
      VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_4_BIT;
  pProperties->limits.framebufferNoAttachmentsSampleCounts =
      VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_4_BIT;
  pProperties->limits.maxColorAttachments = 4;
  pProperties->limits.sampledImageColorSampleCounts =
      VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_4_BIT;
  pProperties->limits.sampledImageIntegerSampleCounts = VK_SAMPLE_COUNT_1_BIT;
  pProperties->limits.sampledImageDepthSampleCounts =
      VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_4_BIT;
  pProperties->limits.sampledImageStencilSampleCounts =
      VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_4_BIT;
  pProperties->limits.storageImageSampleCounts = VK_SAMPLE_COUNT_1_BIT;
  pProperties->limits.maxSampleMaskWords = 1;
  pProperties->limits.timestampComputeAndGraphics = VK_FALSE;
  pProperties->limits.timestampPeriod = 1;
  pProperties->limits.maxClipDistances = 0;
  pProperties->limits.maxCullDistances = 0;
  pProperties->limits.maxCombinedClipAndCullDistances = 0;
  pProperties->limits.discreteQueuePriorities = 2;
  pProperties->limits.pointSizeRange[0] = 1.0;
  pProperties->limits.pointSizeRange[1] = 1.0;
  pProperties->limits.lineWidthRange[0] = 1.0;
  pProperties->limits.lineWidthRange[1] = 1.0;
  pProperties->limits.pointSizeGranularity = 0;
  pProperties->limits.lineWidthGranularity = 0;
  pProperties->limits.strictLines = VK_FALSE;
  pProperties->limits.standardSampleLocations = VK_FALSE;
  pProperties->limits.optimalBufferCopyOffsetAlignment = 1;
  pProperties->limits.optimalBufferCopyRowPitchAlignment = 1;
  pProperties->limits.nonCoherentAtomSize = 256;

  memset(&pProperties->sparseProperties, 0,
         sizeof(VkPhysicalDeviceSparseProperties));
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2 *pProperties)
{
  vkGetPhysicalDeviceProperties(physicalDevice, &pProperties->properties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2KHR(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2 *pProperties)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
    VkQueueFamilyProperties *pQueueFamilyProperties)
{
  if (!pQueueFamilyProperties)
  {
    *pQueueFamilyPropertyCount = 1;
    return;
  }

  if (*pQueueFamilyPropertyCount < 1)
    return;

  *pQueueFamilyPropertyCount = 1;
  pQueueFamilyProperties->queueFlags =
      VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
  pQueueFamilyProperties->queueCount = 1;
  pQueueFamilyProperties->timestampValidBits = 0;
  pQueueFamilyProperties->minImageTransferGranularity = {1, 1, 1};
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties2(
    VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
    VkQueueFamilyProperties2 *pQueueFamilyProperties)
{
  if (!pQueueFamilyProperties)
  {
    *pQueueFamilyPropertyCount = 1;
    return;
  }
  vkGetPhysicalDeviceQueueFamilyProperties(
      physicalDevice, pQueueFamilyPropertyCount,
      &pQueueFamilyProperties->queueFamilyProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties2KHR(
    VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
    VkQueueFamilyProperties2 *pQueueFamilyProperties)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}
