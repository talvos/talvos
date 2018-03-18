// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(
    VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo,
    VkCommandBuffer *pCommandBuffers)

{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(
    VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo)

{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL
vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount,
                     const VkCommandBuffer *pCommandBuffers)

{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDeviceMask(VkCommandBuffer commandBuffer,
                                              uint32_t deviceMask)

{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDeviceMaskKHR(VkCommandBuffer commandBuffer,
                                                 uint32_t deviceMask)

{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(
    VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool)

{
  *pCommandPool = new VkCommandPool_T;
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool,
                     const VkAllocationCallbacks *pAllocator)

{
  for (auto Cmd : commandPool->Pool)
    delete Cmd;
  delete commandPool;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEndCommandBuffer(VkCommandBuffer commandBuffer)

{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(
    VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
    const VkCommandBuffer *pCommandBuffers)

{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(VkQueue queue,
                                             uint32_t submitCount,
                                             const VkSubmitInfo *pSubmits,
                                             VkFence fence)

{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(
    VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags)

{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool(VkDevice device,
                                                  VkCommandPool commandPool,
                                                  VkCommandPoolResetFlags flags)

{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkTrimCommandPool(VkDevice device,
                                             VkCommandPool commandPool,
                                             VkCommandPoolTrimFlags flags)

{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkTrimCommandPoolKHR(VkDevice device,
                                                VkCommandPool commandPool,
                                                VkCommandPoolTrimFlags flags)

{
  TALVOS_ABORT_UNIMPLEMENTED;
}
