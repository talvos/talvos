// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Commands.cpp
/// This file defines the Command base class and its subclasses.

#include <cassert>
#include <cmath>

#include "PipelineExecutor.h"
#include "talvos/Commands.h"
#include "talvos/Device.h"
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
  assert(Format == VK_FORMAT_R8G8B8A8_UNORM);

  // Generate pixel data.
  uint8_t PixelValue[4];
  for (uint32_t i = 0; i < 4; i++)
    PixelValue[i] = (uint8_t)std::round(Color.float32[i] * 255);

  // Loop over ranges in command.
  for (auto &Range : Ranges)
  {
    // Loop over array layers in range.
    for (uint32_t Layer = Range.baseArrayLayer;
         Layer < Range.baseArrayLayer + Range.layerCount; Layer++)
    {
      uint32_t ImageWidth = Extent.width;
      uint32_t ImageHeight = Extent.height;
      uint32_t ImageDepth = Extent.depth;
      uint32_t ImageLayerSize = ImageWidth * ImageHeight * ImageDepth;

      // Store pixel data to each pixel in image.
      for (uint32_t Z = 0; Z < Extent.depth; Z++)
      {
        for (uint32_t Y = 0; Y < Extent.height; Y++)
        {
          for (uint32_t X = 0; X < Extent.width; X++)
          {
            Dev.getGlobalMemory().store(
                Address + (ImageLayerSize * Layer * 4) +
                    (X + ((Y + (Z * Extent.height)) * Extent.width)) * 4,
                4, PixelValue);
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

void CopyImageCommand::runImpl(Device &Dev) const
{
  for (const VkImageCopy &Region : Regions)
  {
    // TODO: Handle array layers.
    assert(Region.srcSubresource.baseArrayLayer == 0 &&
           Region.srcSubresource.layerCount == 1);
    assert(Region.srcSubresource.baseArrayLayer == 0 &&
           Region.dstSubresource.layerCount == 1);

    // TODO: Handle other formats/element sizes
    assert(SrcFormat == VK_FORMAT_R8G8B8A8_UNORM);
    assert(DstFormat == VK_FORMAT_R8G8B8A8_UNORM);
    uint32_t ElementSize = 4;

    uint32_t DstImageWidth = DstSize.width;
    uint32_t DstImageHeight = DstSize.height;
    uint64_t DstBase =
        DstAddr +
        (Region.dstOffset.x +
         (Region.dstOffset.y + (Region.dstOffset.z * DstImageHeight)) *
             DstImageWidth) *
            ElementSize;

    uint32_t SrcImageWidth = SrcSize.width;
    uint32_t SrcImageHeight = SrcSize.height;
    uint64_t SrcBase =
        SrcAddr +
        (Region.srcOffset.x +
         (Region.srcOffset.y + (Region.srcOffset.z * SrcImageHeight)) *
             SrcImageWidth) *
            ElementSize;

    // Copy region one scanline at a time.
    for (uint32_t z = 0; z < Region.extent.depth; z++)
    {
      for (uint32_t y = 0; y < Region.extent.height; y++)
      {
        Memory::copy(DstBase + (((z * DstImageHeight) + y) * DstImageWidth) *
                                   ElementSize,
                     Dev.getGlobalMemory(),
                     SrcBase + (((z * SrcImageHeight) + y) * SrcImageWidth) *
                                   ElementSize,
                     Dev.getGlobalMemory(), Region.extent.width * ElementSize);
      }
    }
  }
}

void CopyImageToBufferCommand::runImpl(Device &Dev) const
{
  for (const VkBufferImageCopy &Region : Regions)
  {
    for (uint32_t LayerOffset = 0;
         LayerOffset < Region.imageSubresource.layerCount; LayerOffset++)
    {
      // TODO: Handle other formats/element sizes
      assert(SrcFormat == VK_FORMAT_R8G8B8A8_UNORM);
      uint32_t ElementSize = 4;

      uint32_t BufferWidth = Region.bufferRowLength ? Region.bufferRowLength
                                                    : Region.imageExtent.width;
      uint32_t BufferHeight = Region.bufferImageHeight
                                  ? Region.bufferImageHeight
                                  : Region.imageExtent.height;
      uint64_t DstBase = DstAddr + Region.bufferOffset;
      DstBase += BufferWidth * BufferHeight * ElementSize * LayerOffset;

      uint32_t ImageWidth = SrcSize.width;
      uint32_t ImageHeight = SrcSize.height;
      uint32_t ImageDepth = SrcSize.depth;
      uint32_t ImageLayerSize = ImageWidth * ImageHeight * ImageDepth;
      uint64_t SrcBase =
          SrcAddr +
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

void CopyBufferToImageCommand::runImpl(Device &Dev) const
{
  for (const VkBufferImageCopy &Region : Regions)
  {
    for (uint32_t LayerOffset = 0;
         LayerOffset < Region.imageSubresource.layerCount; LayerOffset++)
    {
      // TODO: Handle other formats/element sizes
      assert(DstFormat == VK_FORMAT_R8G8B8A8_UNORM);
      uint32_t ElementSize = 4;

      uint32_t BufferWidth = Region.bufferRowLength ? Region.bufferRowLength
                                                    : Region.imageExtent.width;
      uint32_t BufferHeight = Region.bufferImageHeight
                                  ? Region.bufferImageHeight
                                  : Region.imageExtent.height;
      uint64_t SrcBase = SrcAddr + Region.bufferOffset;
      SrcBase += BufferWidth * BufferHeight * ElementSize * LayerOffset;

      uint32_t ImageWidth = DstSize.width;
      uint32_t ImageHeight = DstSize.height;
      uint32_t ImageDepth = DstSize.depth;
      uint32_t ImageLayerSize = ImageWidth * ImageHeight * ImageDepth;
      uint64_t DstBase =
          DstAddr +
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

} // namespace talvos
