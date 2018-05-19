// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Commands.cpp
/// This file defines the Command base class and its subclasses.

#include <cassert>

#include "talvos/Commands.h"
#include "PipelineExecutor.h"
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

void CopyImageToBufferCommand::runImpl(Device &Dev) const
{
  for (const VkBufferImageCopy &Region : Regions)
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

    uint32_t ImageWidth = SrcSize.width;
    uint32_t ImageHeight = SrcSize.height;
    uint64_t SrcBase =
        SrcAddr +
        (Region.imageOffset.x +
         (Region.imageOffset.y + (Region.imageOffset.z * ImageHeight)) *
             ImageWidth) *
            ElementSize;

    // Copy region one scanline at a time.
    for (uint32_t z = Region.imageOffset.z;
         z < Region.imageOffset.z + Region.imageExtent.depth; z++)
    {
      for (uint32_t y = Region.imageOffset.y;
           y < Region.imageOffset.y + Region.imageExtent.height; y++)
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

void DispatchCommand::runImpl(Device &Dev) const
{
  Dev.getPipelineExecutor().run(*this);
}

void DrawCommand::runImpl(Device &Dev) const
{
  Dev.getPipelineExecutor().run(*this);
}

void EndRenderPassCommand::runImpl(Device &Dev) const { RPI->end(); }

void NextSubpassCommand::runImpl(Device &Dev) const { RPI->nextSubpass(); }

} // namespace talvos
