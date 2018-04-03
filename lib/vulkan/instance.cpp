// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

#include <cstring>

// Utility to return the function pointer corresponding to a function name.
PFN_vkVoidFunction getFunctionPointer(const char *Name);

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo,
                 const VkAllocationCallbacks *pAllocator, VkInstance *pInstance)
{
  // Check extensions are supported.
  for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++)
  {
    // TODO: Check whether we actually can support the extension.
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  // Create physical device.
  VkPhysicalDevice_T *Device = new VkPhysicalDevice_T;
  memset(&Device->Features, 0, sizeof(VkPhysicalDeviceFeatures));
  Device->Features.robustBufferAccess = VK_TRUE;

  *pInstance = new VkInstance_T;
  (*pInstance)->Device = Device;

  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator)
{
  delete instance->Device;
  delete instance;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceVersion(uint32_t *pApiVersion)
{
  *pApiVersion = VK_API_VERSION_1_1;
  return VK_SUCCESS;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device,
                                                             const char *pName)
{
  return getFunctionPointer(pName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetInstanceProcAddr(VkInstance instance, const char *pName)
{
  return getFunctionPointer(pName);
}

PFN_vkVoidFunction getFunctionPointer(const char *Name)
{
#define CASE(name)                                                             \
  if (!strcmp(Name, #name))                                                    \
  return (PFN_vkVoidFunction)name

  // Core functions.
  CASE(vkAllocateCommandBuffers);
  CASE(vkAllocateDescriptorSets);
  CASE(vkAllocateMemory);
  CASE(vkBeginCommandBuffer);
  CASE(vkBindBufferMemory);
  CASE(vkBindBufferMemory2);
  CASE(vkBindImageMemory);
  CASE(vkBindImageMemory2);
  CASE(vkCmdBeginQuery);
  CASE(vkCmdBeginRenderPass);
  CASE(vkCmdBindDescriptorSets);
  CASE(vkCmdBindIndexBuffer);
  CASE(vkCmdBindPipeline);
  CASE(vkCmdBindVertexBuffers);
  CASE(vkCmdBlitImage);
  CASE(vkCmdClearAttachments);
  CASE(vkCmdClearColorImage);
  CASE(vkCmdClearDepthStencilImage);
  CASE(vkCmdCopyBuffer);
  CASE(vkCmdCopyBufferToImage);
  CASE(vkCmdCopyImage);
  CASE(vkCmdCopyImageToBuffer);
  CASE(vkCmdCopyQueryPoolResults);
  CASE(vkCmdDispatch);
  CASE(vkCmdDispatchBase);
  CASE(vkCmdDispatchIndirect);
  CASE(vkCmdDraw);
  CASE(vkCmdDrawIndexed);
  CASE(vkCmdDrawIndexedIndirect);
  CASE(vkCmdDrawIndirect);
  CASE(vkCmdEndQuery);
  CASE(vkCmdEndRenderPass);
  CASE(vkCmdExecuteCommands);
  CASE(vkCmdFillBuffer);
  CASE(vkCmdNextSubpass);
  CASE(vkCmdPipelineBarrier);
  CASE(vkCmdPushConstants);
  CASE(vkCmdResetEvent);
  CASE(vkCmdResetQueryPool);
  CASE(vkCmdResolveImage);
  CASE(vkCmdSetBlendConstants);
  CASE(vkCmdSetDepthBias);
  CASE(vkCmdSetDepthBounds);
  CASE(vkCmdSetDeviceMask);
  CASE(vkCmdSetEvent);
  CASE(vkCmdSetLineWidth);
  CASE(vkCmdSetScissor);
  CASE(vkCmdSetStencilCompareMask);
  CASE(vkCmdSetStencilReference);
  CASE(vkCmdSetStencilWriteMask);
  CASE(vkCmdSetViewport);
  CASE(vkCmdUpdateBuffer);
  CASE(vkCmdWaitEvents);
  CASE(vkCmdWriteTimestamp);
  CASE(vkCreateBuffer);
  CASE(vkCreateBufferView);
  CASE(vkCreateCommandPool);
  CASE(vkCreateComputePipelines);
  CASE(vkCreateDescriptorPool);
  CASE(vkCreateDescriptorSetLayout);
  CASE(vkCreateDescriptorUpdateTemplate);
  CASE(vkCreateDevice);
  CASE(vkCreateEvent);
  CASE(vkCreateFence);
  CASE(vkCreateFramebuffer);
  CASE(vkCreateGraphicsPipelines);
  CASE(vkCreateImage);
  CASE(vkCreateImageView);
  CASE(vkCreateInstance);
  CASE(vkCreatePipelineCache);
  CASE(vkCreatePipelineLayout);
  CASE(vkCreateQueryPool);
  CASE(vkCreateRenderPass);
  CASE(vkCreateSampler);
  CASE(vkCreateSamplerYcbcrConversion);
  CASE(vkCreateSemaphore);
  CASE(vkCreateShaderModule);
  CASE(vkDestroyBuffer);
  CASE(vkDestroyBufferView);
  CASE(vkDestroyCommandPool);
  CASE(vkDestroyDescriptorPool);
  CASE(vkDestroyDescriptorSetLayout);
  CASE(vkDestroyDescriptorUpdateTemplate);
  CASE(vkDestroyDevice);
  CASE(vkDestroyEvent);
  CASE(vkDestroyFence);
  CASE(vkDestroyFramebuffer);
  CASE(vkDestroyImage);
  CASE(vkDestroyImageView);
  CASE(vkDestroyInstance);
  CASE(vkDestroyPipeline);
  CASE(vkDestroyPipelineCache);
  CASE(vkDestroyPipelineLayout);
  CASE(vkDestroyQueryPool);
  CASE(vkDestroyRenderPass);
  CASE(vkDestroySampler);
  CASE(vkDestroySamplerYcbcrConversion);
  CASE(vkDestroySemaphore);
  CASE(vkDestroyShaderModule);
  CASE(vkDeviceWaitIdle);
  CASE(vkEndCommandBuffer);
  CASE(vkEnumerateDeviceExtensionProperties);
  CASE(vkEnumerateDeviceLayerProperties);
  CASE(vkEnumerateInstanceExtensionProperties);
  CASE(vkEnumerateInstanceLayerProperties);
  CASE(vkEnumerateInstanceVersion);
  CASE(vkEnumeratePhysicalDeviceGroups);
  CASE(vkEnumeratePhysicalDevices);
  CASE(vkFlushMappedMemoryRanges);
  CASE(vkFreeCommandBuffers);
  CASE(vkFreeDescriptorSets);
  CASE(vkFreeMemory);
  CASE(vkGetBufferMemoryRequirements);
  CASE(vkGetBufferMemoryRequirements2);
  CASE(vkGetDescriptorSetLayoutSupport);
  CASE(vkGetDeviceGroupPeerMemoryFeatures);
  CASE(vkGetDeviceMemoryCommitment);
  CASE(vkGetDeviceProcAddr);
  CASE(vkGetDeviceQueue);
  CASE(vkGetDeviceQueue2);
  CASE(vkGetEventStatus);
  CASE(vkGetFenceStatus);
  CASE(vkGetImageMemoryRequirements);
  CASE(vkGetImageMemoryRequirements2);
  CASE(vkGetImageSparseMemoryRequirements);
  CASE(vkGetImageSparseMemoryRequirements2);
  CASE(vkGetImageSubresourceLayout);
  CASE(vkGetInstanceProcAddr);
  CASE(vkGetPhysicalDeviceExternalBufferProperties);
  CASE(vkGetPhysicalDeviceExternalFenceProperties);
  CASE(vkGetPhysicalDeviceExternalSemaphoreProperties);
  CASE(vkGetPhysicalDeviceFeatures);
  CASE(vkGetPhysicalDeviceFeatures2);
  CASE(vkGetPhysicalDeviceFormatProperties);
  CASE(vkGetPhysicalDeviceFormatProperties2);
  CASE(vkGetPhysicalDeviceImageFormatProperties);
  CASE(vkGetPhysicalDeviceImageFormatProperties2);
  CASE(vkGetPhysicalDeviceMemoryProperties);
  CASE(vkGetPhysicalDeviceMemoryProperties2);
  CASE(vkGetPhysicalDeviceProperties);
  CASE(vkGetPhysicalDeviceProperties2);
  CASE(vkGetPhysicalDeviceQueueFamilyProperties);
  CASE(vkGetPhysicalDeviceQueueFamilyProperties2);
  CASE(vkGetPhysicalDeviceSparseImageFormatProperties);
  CASE(vkGetPhysicalDeviceSparseImageFormatProperties2);
  CASE(vkGetPipelineCacheData);
  CASE(vkGetQueryPoolResults);
  CASE(vkGetRenderAreaGranularity);
  CASE(vkInvalidateMappedMemoryRanges);
  CASE(vkMapMemory);
  CASE(vkMergePipelineCaches);
  CASE(vkQueueBindSparse);
  CASE(vkQueueSubmit);
  CASE(vkQueueWaitIdle);
  CASE(vkResetCommandBuffer);
  CASE(vkResetCommandPool);
  CASE(vkResetDescriptorPool);
  CASE(vkResetEvent);
  CASE(vkResetFences);
  CASE(vkSetEvent);
  CASE(vkTrimCommandPool);
  CASE(vkUnmapMemory);
  CASE(vkUpdateDescriptorSetWithTemplate);
  CASE(vkUpdateDescriptorSets);
  CASE(vkWaitForFences);

  // KHR functions.
  CASE(vkBindBufferMemory2KHR);
  CASE(vkBindImageMemory2KHR);
  CASE(vkCmdDispatchBaseKHR);
  CASE(vkCmdPushDescriptorSetKHR);
  CASE(vkCmdPushDescriptorSetWithTemplateKHR);
  CASE(vkCmdSetDeviceMaskKHR);
  CASE(vkCreateDescriptorUpdateTemplateKHR);
  CASE(vkCreateSamplerYcbcrConversionKHR);
  CASE(vkDestroyDescriptorUpdateTemplateKHR);
  CASE(vkDestroySamplerYcbcrConversionKHR);
  CASE(vkEnumeratePhysicalDeviceGroupsKHR);
  CASE(vkGetBufferMemoryRequirements2KHR);
  CASE(vkGetDescriptorSetLayoutSupportKHR);
  CASE(vkGetDeviceGroupPeerMemoryFeaturesKHR);
  CASE(vkGetFenceFdKHR);
  CASE(vkGetImageMemoryRequirements2KHR);
  CASE(vkGetImageSparseMemoryRequirements2KHR);
  CASE(vkGetMemoryFdKHR);
  CASE(vkGetMemoryFdPropertiesKHR);
  CASE(vkGetPhysicalDeviceExternalBufferPropertiesKHR);
  CASE(vkGetPhysicalDeviceExternalFencePropertiesKHR);
  CASE(vkGetPhysicalDeviceExternalSemaphorePropertiesKHR);
  CASE(vkGetPhysicalDeviceFeatures2KHR);
  CASE(vkGetPhysicalDeviceFormatProperties2KHR);
  CASE(vkGetPhysicalDeviceImageFormatProperties2KHR);
  CASE(vkGetPhysicalDeviceMemoryProperties2KHR);
  CASE(vkGetPhysicalDeviceProperties2KHR);
  CASE(vkGetPhysicalDeviceQueueFamilyProperties2KHR);
  CASE(vkGetPhysicalDeviceSparseImageFormatProperties2KHR);
  CASE(vkGetSemaphoreFdKHR);
  CASE(vkImportFenceFdKHR);
  CASE(vkImportSemaphoreFdKHR);
  CASE(vkTrimCommandPoolKHR);
  CASE(vkUpdateDescriptorSetWithTemplateKHR);

#undef CASE

  return nullptr;
}
