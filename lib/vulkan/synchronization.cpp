// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

#if defined(_WIN32) && !defined(__MINGW32__)
#include <time.h>
#else
#include <sys/time.h>
#endif

#include "talvos/Commands.h"
#include "talvos/Queue.h"

/// Utility to return the current time in nanoseconds since the epoch.
double now()
{
#if defined(_WIN32) && !defined(__MINGW32__)
  return time(NULL) * 1e9;
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_usec * 1e3 + tv.tv_sec * 1e9;
#endif
}

VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(
    VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
    uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
    uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier *pBufferMemoryBarriers,
    uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier *pImageMemoryBarriers)
{
  // TODO: Need to handle this properly?
}

VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent(VkCommandBuffer commandBuffer,
                                           VkEvent event,
                                           VkPipelineStageFlags stageMask)
{
  commandBuffer->Commands.push_back(
      new talvos::ResetEventCommand(&event->Signaled));
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent(VkCommandBuffer commandBuffer,
                                         VkEvent event,
                                         VkPipelineStageFlags stageMask)
{
  commandBuffer->Commands.push_back(
      new talvos::SetEventCommand(&event->Signaled));
}

VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents(
    VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
    uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier *pBufferMemoryBarriers,
    uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier *pImageMemoryBarriers)
{
  // TODO: Do we need to handle barriers and stage masks?

  std::vector<bool *> Flags;
  for (uint32_t i = 0; i < eventCount; i++)
    Flags.push_back(&pEvents[i]->Signaled);

  commandBuffer->Commands.push_back(new talvos::WaitEventsCommand(Flags));
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateEvent(VkDevice device, const VkEventCreateInfo *pCreateInfo,
              const VkAllocationCallbacks *pAllocator, VkEvent *pEvent)
{
  *pEvent = new VkEvent_T;
  (*pEvent)->Signaled = false;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo,
              const VkAllocationCallbacks *pAllocator, VkFence *pFence)
{
  *pFence = new VkFence_T;
  (*pFence)->Signaled = pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSemaphore(
    VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore)
{
  *pSemaphore = new VkSemaphore_T;
  (*pSemaphore)->Signaled = false;
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyEvent(
    VkDevice device, VkEvent event, const VkAllocationCallbacks *pAllocator)
{
  delete event;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyFence(
    VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator)
{
  delete fence;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroySemaphore(VkDevice device, VkSemaphore semaphore,
                   const VkAllocationCallbacks *pAllocator)
{
  delete semaphore;
}

VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(VkDevice device)
{
  device->Queue->waitIdle();
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetEventStatus(VkDevice device, VkEvent event)
{
  return event->Signaled ? VK_EVENT_SET : VK_EVENT_RESET;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceFdKHR(
    VkDevice device, const VkFenceGetFdInfoKHR *pGetFdInfo, int *pFd)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceStatus(VkDevice device, VkFence fence)
{
  return fence->Signaled ? VK_SUCCESS : VK_NOT_READY;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSemaphoreFdKHR(
    VkDevice device, const VkSemaphoreGetFdInfoKHR *pGetFdInfo, int *pFd)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL vkImportFenceFdKHR(
    VkDevice device, const VkImportFenceFdInfoKHR *pImportFenceFdInfo)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL vkImportSemaphoreFdKHR(
    VkDevice device, const VkImportSemaphoreFdInfoKHR *pImportSemaphoreFdInfo)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(VkQueue queue)
{
  queue->Queue->waitIdle();
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetEvent(VkDevice device, VkEvent event)
{
  event->Signaled = false;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetFences(VkDevice device,
                                             uint32_t fenceCount,
                                             const VkFence *pFences)
{
  for (uint32_t i = 0; i < fenceCount; i++)
  {
    pFences[i]->Signaled = false;
  }
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkSetEvent(VkDevice device, VkEvent event)
{
  event->Signaled = true;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(VkDevice device,
                                               uint32_t fenceCount,
                                               const VkFence *pFences,
                                               VkBool32 waitAll,
                                               uint64_t timeout)
{
  double Start = now();
  while (true)
  {
    if (Start + timeout <= now())
      return VK_TIMEOUT;

    uint32_t SignalCount = 0;
    for (uint32_t i = 0; i < fenceCount; i++)
    {
      if (pFences[i]->Signaled)
        SignalCount++;
    }
    if (waitAll ? SignalCount == fenceCount : SignalCount > 0)
      return VK_SUCCESS;
  }
}
