// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Commands.cpp
/// This file defines the Command base class and its subclasses.

#include <cassert>

#include "PipelineExecutor.h"
#include "talvos/Commands.h"
#include "talvos/Device.h"
#include "talvos/Image.h"
#include "talvos/Memory.h"
#include "talvos/RenderPass.h"

namespace talvos
{

void Command::run(Device &Dev) const
{
  Dev.reportCommandBegin(this);
  runImpl(Dev);
  Dev.reportCommandComplete(this);
}

void BeginRenderPassCommand::runImpl(Device &Dev) const { RPI->begin(); }

void ClearColorImageCommand::runImpl(Device &Dev) const
{
  // TODO: Handle mip levels and other formats.
  assert(Ranges[0].baseMipLevel == 0 && Ranges[0].levelCount == 1);

  // Loop over ranges in command.
  for (auto &Range : Ranges)
  {
    // TODO: Handle VK_REMAINING_ARRAY_LAYERS
    // Loop over array layers in range.
    for (uint32_t Layer = Range.baseArrayLayer;
         Layer < Range.baseArrayLayer + Range.layerCount; Layer++)
    {
      // Store pixel data to each pixel in image.
      for (uint32_t Z = 0; Z < DstImage.getDepth(); Z++)
      {
        for (uint32_t Y = 0; Y < DstImage.getHeight(); Y++)
        {
          for (uint32_t X = 0; X < DstImage.getWidth(); X++)
          {
            DstImage.write(Color, DstImage.getTexelAddress(X, Y, Z, Layer));
          }
        }
      }
    }
  }
}

void CopyBufferCommand::runImpl(Device &Dev) const
{
  for (const VkBufferCopy &Region : Regions)
  {
    Memory::copy(DstAddr + Region.dstOffset, Dev.getGlobalMemory(),
                 SrcAddr + Region.srcOffset, Dev.getGlobalMemory(),
                 Region.size);
  }
}

void FillBufferCommand::runImpl(Device &Dev) const
{
  for (uint64_t Address = Base; Address < Base + NumBytes; Address += 4)
    Dev.getGlobalMemory().store(Address, 4, (const uint8_t *)&Data);
}

void CopyBufferToImageCommand::runImpl(Device &Dev) const
{
  uint32_t ElementSize = DstImage.getElementSize();

  for (const VkBufferImageCopy &Region : Regions)
  {
    uint32_t MipLevel = Region.imageSubresource.mipLevel;
    uint64_t MipOffset = DstImage.getMipLevelOffset(MipLevel);
    uint32_t ImageWidth = DstImage.getWidth(MipLevel);
    uint32_t ImageHeight = DstImage.getHeight(MipLevel);
    uint32_t ImageDepth = DstImage.getDepth(MipLevel);
    uint32_t ImageLayerSize = ImageWidth * ImageHeight * ImageDepth;

    // TODO: Handle VK_REMAINING_ARRAY_LAYERS
    for (uint32_t LayerOffset = 0;
         LayerOffset < Region.imageSubresource.layerCount; LayerOffset++)
    {
      uint32_t BufferWidth = Region.bufferRowLength ? Region.bufferRowLength
                                                    : Region.imageExtent.width;
      uint32_t BufferHeight = Region.bufferImageHeight
                                  ? Region.bufferImageHeight
                                  : Region.imageExtent.height;
      uint64_t SrcBase = SrcAddr + Region.bufferOffset;
      SrcBase += BufferWidth * BufferHeight * ElementSize * LayerOffset;

      uint64_t DstBase =
          DstImage.getAddress() + MipOffset +
          (Region.imageOffset.x +
           (Region.imageOffset.y + (Region.imageOffset.z * ImageHeight)) *
               ImageWidth) *
              ElementSize;
      DstBase += ImageLayerSize * ElementSize *
                 (Region.imageSubresource.baseArrayLayer + LayerOffset);

      // Copy region one scanline at a time.
      for (uint32_t z = 0; z < Region.imageExtent.depth; z++)
      {
        for (uint32_t y = 0; y < Region.imageExtent.height; y++)
        {
          Memory::copy(
              DstBase + (((z * ImageHeight) + y) * ImageWidth) * ElementSize,
              Dev.getGlobalMemory(),
              SrcBase + (((z * BufferHeight) + y) * BufferWidth) * ElementSize,
              Dev.getGlobalMemory(), Region.imageExtent.width * ElementSize);
        }
      }
    }
  }
}

void CopyImageCommand::runImpl(Device &Dev) const
{
  uint32_t SrcElementSize = SrcImage.getElementSize();
  uint32_t DstElementSize = DstImage.getElementSize();

  assert(SrcElementSize == DstElementSize);
  for (const VkImageCopy &Region : Regions)
  {
    // TODO: Handle mip levels.
    assert(Region.srcSubresource.mipLevel == 0 &&
           Region.dstSubresource.mipLevel == 0);

    // TODO: Handle array layers.
    assert(Region.srcSubresource.baseArrayLayer == 0 &&
           Region.srcSubresource.layerCount == 1);
    assert(Region.srcSubresource.baseArrayLayer == 0 &&
           Region.dstSubresource.layerCount == 1);

    uint32_t DstImageWidth = DstImage.getWidth();
    uint32_t DstImageHeight = DstImage.getHeight();
    uint64_t DstBase =
        DstImage.getAddress() +
        (Region.dstOffset.x +
         (Region.dstOffset.y + (Region.dstOffset.z * DstImageHeight)) *
             DstImageWidth) *
            DstElementSize;

    uint32_t SrcImageWidth = SrcImage.getWidth();
    uint32_t SrcImageHeight = SrcImage.getHeight();
    uint64_t SrcBase =
        SrcImage.getAddress() +
        (Region.srcOffset.x +
         (Region.srcOffset.y + (Region.srcOffset.z * SrcImageHeight)) *
             SrcImageWidth) *
            SrcElementSize;

    // Copy region one scanline at a time.
    for (uint32_t z = 0; z < Region.extent.depth; z++)
    {
      for (uint32_t y = 0; y < Region.extent.height; y++)
      {
        Memory::copy(DstBase + (((z * DstImageHeight) + y) * DstImageWidth) *
                                   DstElementSize,
                     Dev.getGlobalMemory(),
                     SrcBase + (((z * SrcImageHeight) + y) * SrcImageWidth) *
                                   SrcElementSize,
                     Dev.getGlobalMemory(),
                     Region.extent.width * SrcElementSize);
      }
    }
  }
}

void CopyImageToBufferCommand::runImpl(Device &Dev) const
{
  uint32_t ElementSize = SrcImage.getElementSize();

  for (const VkBufferImageCopy &Region : Regions)
  {
    uint32_t MipLevel = Region.imageSubresource.mipLevel;
    uint64_t MipOffset = SrcImage.getMipLevelOffset(MipLevel);
    uint32_t ImageWidth = SrcImage.getWidth(MipLevel);
    uint32_t ImageHeight = SrcImage.getHeight(MipLevel);
    uint32_t ImageDepth = SrcImage.getDepth(MipLevel);
    uint32_t ImageLayerSize = ImageWidth * ImageHeight * ImageDepth;

    // TODO: Handle VK_REMAINING_ARRAY_LAYERS
    for (uint32_t LayerOffset = 0;
         LayerOffset < Region.imageSubresource.layerCount; LayerOffset++)
    {
      uint32_t BufferWidth = Region.bufferRowLength ? Region.bufferRowLength
                                                    : Region.imageExtent.width;
      uint32_t BufferHeight = Region.bufferImageHeight
                                  ? Region.bufferImageHeight
                                  : Region.imageExtent.height;
      uint64_t DstBase = DstAddr + Region.bufferOffset;
      DstBase += BufferWidth * BufferHeight * ElementSize * LayerOffset;

      uint64_t SrcBase =
          SrcImage.getAddress() + MipOffset +
          (Region.imageOffset.x +
           (Region.imageOffset.y + (Region.imageOffset.z * ImageHeight)) *
               ImageWidth) *
              ElementSize;
      SrcBase += ImageLayerSize * ElementSize *
                 (Region.imageSubresource.baseArrayLayer + LayerOffset);

      // Copy region one scanline at a time.
      for (uint32_t z = 0; z < Region.imageExtent.depth; z++)
      {
        for (uint32_t y = 0; y < Region.imageExtent.height; y++)
        {
          Memory::copy(
              DstBase + (((z * BufferHeight) + y) * BufferWidth) * ElementSize,
              Dev.getGlobalMemory(),
              SrcBase + (((z * ImageHeight) + y) * ImageWidth) * ElementSize,
              Dev.getGlobalMemory(), Region.imageExtent.width * ElementSize);
        }
      }
    }
  }
}

void DispatchCommand::runImpl(Device &Dev) const
{
  Dev.getPipelineExecutor().run(*this);
}

void DrawCommand::runImpl(Device &Dev) const
{
  Dev.getPipelineExecutor().run(*this);
}

void DrawIndexedCommand::runImpl(Device &Dev) const
{
  Dev.getPipelineExecutor().run(*this);
}

void EndRenderPassCommand::runImpl(Device &Dev) const { RPI->end(); }

void NextSubpassCommand::runImpl(Device &Dev) const { RPI->nextSubpass(); }

void ResetEventCommand::runImpl(Device &Dev) const { *Event = false; }

void SetEventCommand::runImpl(Device &Dev) const { *Event = true; }

void WaitEventsCommand::runImpl(Device &Dev) const
{
  // Wait for all events to be set.
  for (volatile bool *Event : Events)
  {
    while (!*Event)
      ;
  }
}

} // namespace talvos
