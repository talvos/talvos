// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Commands.cpp
/// This file defines the Command base class and its subclasses.

#include <algorithm>
#include <cassert>
#include <cmath>

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

void BlitImageCommand::runImpl(Device &Dev) const
{
  // TODO: Handle linear filtering.
  assert(Filter == VK_FILTER_NEAREST);

  for (const VkImageBlit &Region : Regions)
  {
    uint32_t SrcLevel = Region.srcSubresource.mipLevel;
    uint32_t DstLevel = Region.dstSubresource.mipLevel;

    // TODO: Handle array layers.
    assert(Region.srcSubresource.baseArrayLayer == 0 &&
           Region.srcSubresource.layerCount == 1);
    assert(Region.srcSubresource.baseArrayLayer == 0 &&
           Region.dstSubresource.layerCount == 1);

    int32_t XMin = std::min(Region.dstOffsets[0].x, Region.dstOffsets[1].x);
    int32_t XMax = std::max(Region.dstOffsets[0].x, Region.dstOffsets[1].x);
    int32_t YMin = std::min(Region.dstOffsets[0].y, Region.dstOffsets[1].y);
    int32_t YMax = std::max(Region.dstOffsets[0].y, Region.dstOffsets[1].y);
    int32_t ZMin = std::min(Region.dstOffsets[0].z, Region.dstOffsets[1].z);
    int32_t ZMax = std::max(Region.dstOffsets[0].z, Region.dstOffsets[1].z);

    // Blit region one texel at a time.
    for (int32_t Z = ZMin; Z < ZMax; Z++)
    {
      for (int32_t Y = YMin; Y < YMax; Y++)
      {
        for (int32_t X = XMin; X < XMax; X++)
        {
          // Generate scaled coordinate for source image.
          float U = X + 0.5f - Region.dstOffsets[0].x;
          float V = Y + 0.5f - Region.dstOffsets[0].y;
          float W = Z + 0.5f - Region.dstOffsets[0].z;
          U *= (float)(Region.srcOffsets[1].x - Region.srcOffsets[0].x) /
               (float)(Region.dstOffsets[1].x - Region.dstOffsets[0].x);
          V *= (float)(Region.srcOffsets[1].y - Region.srcOffsets[0].y) /
               (float)(Region.dstOffsets[1].y - Region.dstOffsets[0].y);
          W *= (float)(Region.srcOffsets[1].z - Region.srcOffsets[0].z) /
               (float)(Region.dstOffsets[1].z - Region.dstOffsets[0].z);
          U += Region.srcOffsets[0].x;
          V += Region.srcOffsets[0].y;
          W += Region.srcOffsets[0].z;
          int32_t SrcX = std::clamp<int32_t>(std::floor(U), 0,
                                             SrcImage.getExtent().width - 1);
          int32_t SrcY = std::clamp<int32_t>(std::floor(V), 0,
                                             SrcImage.getExtent().height - 1);
          int32_t SrcZ = std::clamp<int32_t>(std::floor(W), 0,
                                             SrcImage.getExtent().depth - 1);

          // Copy texel from source to destination.
          Image::Texel T;
          SrcImage.read(
              T, SrcImage.getTexelAddress(SrcX, SrcY, SrcZ, 0, SrcLevel));
          DstImage.write(T, DstImage.getTexelAddress(X, Y, Z, 0, DstLevel));
        }
      }
    }
  }
}

void ClearAttachmentCommand::runImpl(Device &Dev) const
{
  // Loop over attachments.
  for (auto &Attachment : ClearAttachments)
  {
    // TODO: Handle clearing depth/stencil attachments.
    assert(Attachment.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);

    // Get target image view.
    const Subpass &SP = RPI.getRenderPass().getSubpass(RPI.getSubpassIndex());
    uint32_t AttachIndex = SP.ColorAttachments[Attachment.colorAttachment];
    ImageView *DstImage = RPI.getFramebuffer().getAttachments()[AttachIndex];

    // Loop over clear regions.
    for (auto &Rect : ClearRects)
    {
      // Compute start and end coordinates.
      uint32_t XMin = Rect.rect.offset.x;
      uint32_t XMax = XMin + Rect.rect.extent.width;
      uint32_t YMin = Rect.rect.offset.y;
      uint32_t YMax = YMin + Rect.rect.extent.height;
      uint32_t LastLayer = Rect.baseArrayLayer + Rect.layerCount - 1;

      // Loop over array layers.
      for (uint32_t Layer = Rect.baseArrayLayer; Layer <= LastLayer; Layer++)
      {
        // Store pixel data to each pixel in image.
        for (uint32_t Y = YMin; Y < YMax; Y++)
        {
          for (uint32_t X = XMin; X < XMax; X++)
          {
            DstImage->write(Attachment.clearValue.color, X, Y, 0, Layer);
          }
        }
      }
    }
  }
}

void ClearColorImageCommand::runImpl(Device &Dev) const
{
  // Loop over ranges in command.
  for (auto &Range : Ranges)
  {
    // Compute last mip level in range.
    uint32_t LastLevel = Range.baseMipLevel + Range.levelCount - 1;
    if (Range.levelCount == VK_REMAINING_MIP_LEVELS)
      LastLevel = DstImage.getNumMipLevels() - 1;

    // Compute last array layer in range.
    uint32_t LastLayer = Range.baseArrayLayer + Range.layerCount - 1;
    if (Range.layerCount == VK_REMAINING_ARRAY_LAYERS)
      LastLayer = DstImage.getNumArrayLayers() - 1;

    // Loop over mip levels.
    for (uint32_t Level = Range.baseMipLevel; Level <= LastLevel; Level++)
    {
      // Loop over array layers.
      for (uint32_t Layer = Range.baseArrayLayer; Layer <= LastLayer; Layer++)
      {
        // Store pixel data to each pixel in image.
        for (uint32_t Z = 0; Z < DstImage.getDepth(Level); Z++)
        {
          for (uint32_t Y = 0; Y < DstImage.getHeight(Level); Y++)
          {
            for (uint32_t X = 0; X < DstImage.getWidth(Level); X++)
            {
              DstImage.write(Color,
                             DstImage.getTexelAddress(X, Y, Z, Layer, Level));
            }
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

void FillBufferCommand::runImpl(Device &Dev) const
{
  for (uint64_t Address = Base; Address < Base + NumBytes; Address += 4)
    Dev.getGlobalMemory().store(Address, 4, (const uint8_t *)&Data);
}

void NextSubpassCommand::runImpl(Device &Dev) const { RPI->nextSubpass(); }

void ResetEventCommand::runImpl(Device &Dev) const { *Event = false; }

void SetEventCommand::runImpl(Device &Dev) const { *Event = true; }

UpdateBufferCommand::UpdateBufferCommand(uint64_t Base, uint64_t NumBytes,
                                         const void *Data)
    : Command(UPDATE_BUFFER), Base(Base), NumBytes(NumBytes)
{
  this->Data = new uint8_t[NumBytes];
  memcpy(this->Data, Data, NumBytes);
}

UpdateBufferCommand::~UpdateBufferCommand() { delete[] Data; }

void UpdateBufferCommand::runImpl(Device &Dev) const
{
  Dev.getGlobalMemory().store(Base, NumBytes, Data);
}

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
