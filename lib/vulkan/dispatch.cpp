// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

#include "talvos/Commands.h"

VKAPI_ATTR void VKAPI_CALL vkCmdDispatch(VkCommandBuffer commandBuffer,
                                         uint32_t groupCountX,
                                         uint32_t groupCountY,
                                         uint32_t groupCountZ)
{
  // TODO: These dispatch commands are currently never deleted.
  commandBuffer->Commands.push_back(new talvos::DispatchCommand(
      commandBuffer->PipelineCompute->ComputePipeline,
      {groupCountX, groupCountY, groupCountZ},
      commandBuffer->DescriptorSetsCompute));
}

VKAPI_ATTR void VKAPI_CALL vkCmdDispatchBase(
    VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY,
    uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY,
    uint32_t groupCountZ)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkCmdDispatchBaseKHR(
    VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY,
    uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY,
    uint32_t groupCountZ)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkCmdDispatchIndirect(VkCommandBuffer commandBuffer,
                                                 VkBuffer buffer,
                                                 VkDeviceSize offset)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}
