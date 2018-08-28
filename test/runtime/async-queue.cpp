//
// Tests that the asynchronous queue behaves properly with events and fences.
//

#include "common.h"

#include <iostream>

#if defined(_WIN32) && !defined(__MINGW32__)
#include <windows.h>
#else
#include <unistd.h>
#endif

int main(int argc, char *argv[])
{
  VkResult Result;
  VkCommandBuffer CommandBuffer;
  VkFence Fence;
  VkEvent Event;

  // Create test context.
  TestContext Context("test/async-queue");

  // Allocate command buffer.
  VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, NULL, Context.CommandPool,
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1};
  Result = vkAllocateCommandBuffers(Context.Device, &CommandBufferAllocateInfo,
                                    &CommandBuffer);
  check(Result, "creating command buffer");

  // Create fence.
  VkFenceCreateInfo FenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, 0};
  Result = vkCreateFence(Context.Device, &FenceInfo, NULL, &Fence);
  check(Result, "creating fence");

  // Create event.
  VkEventCreateInfo EventInfo = {VK_STRUCTURE_TYPE_EVENT_CREATE_INFO, NULL, 0};
  Result = vkCreateEvent(Context.Device, &EventInfo, NULL, &Event);
  check(Result, "creating event");

  // Test that fence signals when submitting zero commands.
  {
    // Submit empty set of commands.
    VkSubmitInfo SubmitInfo = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, NULL, 0, NULL, 0, NULL};
    Result = vkQueueSubmit(Context.Queue, 1, &SubmitInfo, Fence);
    check(Result, "submitting command");

    // Wait for commands to complete.
    vkQueueWaitIdle(Context.Queue);
    check(Result, "waiting for queue to be idle");

    // Check that fence has signaled.
    Result = vkGetFenceStatus(Context.Device, Fence);
    if (Result != VK_SUCCESS)
    {
      std::cerr << "Fence failed to signal." << std::endl;
      exit(1);
    }
  }

  // Test that queue blocks and application progresses when waiting for event.
  {
    vkResetFences(Context.Device, 1, &Fence);
    vkResetEvent(Context.Device, Event);

    // Begin recording commands.
    VkCommandBufferBeginInfo BeginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, NULL};
    Result = vkBeginCommandBuffer(CommandBuffer, &BeginInfo);
    check(Result, "begin command buffer");

    // Add command to block for an event.
    vkCmdWaitEvents(CommandBuffer, 1, &Event, 0, 0, 0, NULL, 0, NULL, 0, NULL);

    // Finish recording commands.
    Result = vkEndCommandBuffer(CommandBuffer);
    check(Result, "end command buffer");

    // Submit command buffer to queue with fence.
    VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO,
                               NULL,
                               0,
                               NULL,
                               NULL,
                               1,
                               &CommandBuffer,
                               0,
                               NULL};
    Result = vkQueueSubmit(Context.Queue, 1, &SubmitInfo, Fence);
    check(Result, "submitting command");

    // Allow for async queue to progress.
    sleep(1);

    // Check that fence has not signaled yet.
    Result = vkGetFenceStatus(Context.Device, Fence);
    if (Result != VK_NOT_READY)
    {
      std::cerr << "Fence signaled prematurely." << std::endl;
      exit(1);
    }

    // Trigger event to allow commands to complete.
    Result = vkSetEvent(Context.Device, Event);
    check(Result, "setting event");

    // Wait for commands to complete.
    vkQueueWaitIdle(Context.Queue);
    check(Result, "waiting for queue to be idle");

    // Check that fence has now signaled.
    Result = vkGetFenceStatus(Context.Device, Fence);
    if (Result != VK_SUCCESS)
    {
      std::cerr << "Fence failed to signal." << std::endl;
      exit(1);
    }
  }

  // Cleanup.
  vkDestroyEvent(Context.Device, Event, NULL);
  vkDestroyFence(Context.Device, Fence, NULL);
  vkFreeCommandBuffers(Context.Device, Context.CommandPool, 1, &CommandBuffer);

  return 0;
}
