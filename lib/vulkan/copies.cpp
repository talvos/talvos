// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

VKAPI_ATTR void VKAPI_CALL
vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage,
               VkImageLayout srcImageLayout, VkImage dstImage,
               VkImageLayout dstImageLayout, uint32_t regionCount,
               const VkImageBlit *pRegions, VkFilter filter)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer(VkCommandBuffer commandBuffer,
                                           VkBuffer srcBuffer,
                                           VkBuffer dstBuffer,
                                           uint32_t regionCount,
                                           const VkBufferCopy *pRegions)
{
  std::vector<VkBufferCopy> Regions(pRegions, pRegions + regionCount);
  commandBuffer->Commands.push_back(new talvos::CopyBufferCommand(
      srcBuffer->Address, dstBuffer->Address, Regions));
}

VKAPI_ATTR void VKAPI_CALL
vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer,
                       VkImage dstImage, VkImageLayout dstImageLayout,
                       uint32_t regionCount, const VkBufferImageCopy *pRegions)
{
  std::vector<VkBufferImageCopy> Regions(pRegions, pRegions + regionCount);
  commandBuffer->Commands.push_back(new talvos::CopyBufferToImageCommand(
      srcBuffer->Address, *dstImage->Image, Regions));
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage(VkCommandBuffer commandBuffer,
                                          VkImage srcImage,
                                          VkImageLayout srcImageLayout,
                                          VkImage dstImage,
                                          VkImageLayout dstImageLayout,
                                          uint32_t regionCount,
                                          const VkImageCopy *pRegions)
{
  std::vector<VkImageCopy> Regions(pRegions, pRegions + regionCount);
  commandBuffer->Commands.push_back(new talvos::CopyImageCommand(
      *srcImage->Image, *dstImage->Image, Regions));
}

VKAPI_ATTR void VKAPI_CALL
vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage,
                       VkImageLayout srcImageLayout, VkBuffer dstBuffer,
                       uint32_t regionCount, const VkBufferImageCopy *pRegions)
{
  std::vector<VkBufferImageCopy> Regions(pRegions, pRegions + regionCount);
  commandBuffer->Commands.push_back(new talvos::CopyImageToBufferCommand(
      *srcImage->Image, dstBuffer->Address, Regions));
}

VKAPI_ATTR void VKAPI_CALL vkCmdResolveImage(VkCommandBuffer commandBuffer,
                                             VkImage srcImage,
                                             VkImageLayout srcImageLayout,
                                             VkImage dstImage,
                                             VkImageLayout dstImageLayout,
                                             uint32_t regionCount,
                                             const VkImageResolve *pRegions)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}
