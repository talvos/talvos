// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

#include "talvos/Device.h"

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(
    VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo,
    VkCommandBuffer *pCommandBuffers)
{
  for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++)
  {
    pCommandBuffers[i] = new VkCommandBuffer_T;
    pAllocateInfo->commandPool->Pool.insert(pCommandBuffers[i]);
  }
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(
    VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo)
{
  // TODO: Implement?
  return VK_SUCCESS;
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
  // TODO: Implement?
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(
    VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
    const VkCommandBuffer *pCommandBuffers)
{
  for (uint32_t i = 0; i < commandBufferCount; i++)
  {
    commandPool->Pool.erase(pCommandBuffers[i]);
    delete pCommandBuffers[i];
  }
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(VkQueue queue,
                                             uint32_t submitCount,
                                             const VkSubmitInfo *pSubmits,
                                             VkFence fence)
{
  for (uint32_t s = 0; s < submitCount; s++)
  {
    for (uint32_t c = 0; c < pSubmits[s].commandBufferCount; c++)
    {
      for (auto Command : pSubmits[s].pCommandBuffers[c]->Commands)
      {
        Command->run(*queue->Device->Device);
      }
    }
  }
  if (fence)
    fence->Signaled = true;
  return VK_SUCCESS;
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
  for (auto Cmd : commandPool->Pool)
  {
    Cmd->PipelineGraphics = nullptr;
    Cmd->PipelineCompute = nullptr;
    Cmd->DescriptorSetsGraphics.clear();
    Cmd->DescriptorSetsCompute.clear();
    Cmd->VertexBindings.clear();
    Cmd->Commands.clear();
  }
  return VK_SUCCESS;
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
