// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

#include "talvos/Queue.h"

void resetCommandBuffer(VkCommandBuffer Cmd)
{
  Cmd->PipelineContext.clear();
  Cmd->RenderPassInstance.reset();
  Cmd->IndexBufferAddress = 0;
  Cmd->IndexType = VK_INDEX_TYPE_MAX_ENUM;
  Cmd->Commands.clear();
}

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
  resetCommandBuffer(commandBuffer);

  if (pBeginInfo->flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT)
  {
    // TODO: Handle non-zero subpass index here
    assert(pBeginInfo->pInheritanceInfo->subpass == 0);

    // TODO: Handle this (need to clone draw commands to update RPI)
    assert(!(pBeginInfo->flags & VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT));
  }

  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount,
                     const VkCommandBuffer *pCommandBuffers)
{
  for (uint32_t i = 0; i < commandBufferCount; i++)
  {
    for (auto Cmd : pCommandBuffers[i]->Commands)
    {
      // Update the render pass instance for draw commands.
      if (Cmd->getType() == talvos::Command::DRAW ||
          Cmd->getType() == talvos::Command::DRAW_INDEXED)
        static_cast<talvos::DrawCommandBase *>(Cmd)->setRenderPassInstance(
            commandBuffer->RenderPassInstance);

      // Add the command to the primary command buffer.
      commandBuffer->Commands.push_back(Cmd);
    }
  }
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDeviceMask(VkCommandBuffer commandBuffer,
                                              uint32_t deviceMask)
{
  // TODO: Support groups with multiple devices
  assert(deviceMask == 0x1);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDeviceMaskKHR(VkCommandBuffer commandBuffer,
                                                 uint32_t deviceMask)
{
  vkCmdSetDeviceMask(commandBuffer, deviceMask);
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
  if (commandPool)
  {
    for (auto Cmd : commandPool->Pool)
      delete Cmd;
    delete commandPool;
  }
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
  // Build full list of commands in this submission.
  std::vector<talvos::Command *> Commands;
  for (uint32_t s = 0; s < submitCount; s++)
  {
    for (uint32_t c = 0; c < pSubmits[s].commandBufferCount; c++)
    {
      Commands.insert(Commands.end(),
                      pSubmits[s].pCommandBuffers[c]->Commands.begin(),
                      pSubmits[s].pCommandBuffers[c]->Commands.end());
    }
  }

  // Submit commands with fence.
  queue->Queue->submit(Commands, fence ? &fence->Signaled : nullptr);

  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(
    VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags)
{
  resetCommandBuffer(commandBuffer);
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool(VkDevice device,
                                                  VkCommandPool commandPool,
                                                  VkCommandPoolResetFlags flags)
{
  for (auto Cmd : commandPool->Pool)
    resetCommandBuffer(Cmd);

  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkTrimCommandPool(VkDevice device,
                                             VkCommandPool commandPool,
                                             VkCommandPoolTrimFlags flags)
{}

VKAPI_ATTR void VKAPI_CALL vkTrimCommandPoolKHR(VkDevice device,
                                                VkCommandPool commandPool,
                                                VkCommandPoolTrimFlags flags)
{
  vkTrimCommandPool(device, commandPool, flags);
}
