// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file RenderPass.cpp
/// This file defines the RenderPass class and related data structures.

#include <cassert>
#include <cmath>

#include "talvos/Device.h"
#include "talvos/Memory.h"
#include "talvos/RenderPass.h"

namespace talvos
{

const VkAttachmentDescription &RenderPass::getAttachment(uint32_t Index) const
{
  assert(Index < Attachments.size());
  return Attachments[Index];
}

const Subpass &RenderPass::getSubpass(uint32_t Index) const
{
  assert(Index < Subpasses.size());
  return Subpasses[Index];
}

void RenderPassInstance::begin()
{
  // TODO: Mutex instead?
  assert(!Rendering && "render pass overlapping not allowed");

  Rendering = true;

  AttachmentsInitialized.assign(RP.getNumAttachments(), false);

  SubpassIndex = 0;
  beginSubpass();
}

void RenderPassInstance::beginSubpass()
{
  // TODO: Handle stencil/preserve/resolve attachments too

  const Subpass &Subpass = RP.getSubpass(SubpassIndex);

  // Loop over color attachments in the subpass, clearing them as necessary.
  for (uint32_t AttachIndex = 0; AttachIndex < Subpass.ColorAttachments.size();
       AttachIndex++)
  {
    uint32_t AttachRef = Subpass.ColorAttachments[AttachIndex];

    // Skip unused attachments and those that have been previously initialized.
    if (AttachRef == VK_ATTACHMENT_UNUSED)
      continue;
    if (AttachmentsInitialized[AttachRef])
      continue;

    const VkAttachmentDescription &Attachment = RP.getAttachment(AttachRef);
    AttachmentsInitialized[AttachRef] = true;

    // Skip attachments if not clearing them.
    if (Attachment.loadOp != VK_ATTACHMENT_LOAD_OP_CLEAR)
      continue;

    // TODO: Handle other image formats.
    assert(Attachment.format == VK_FORMAT_R8G8B8A8_UNORM);

    // Generate integer clear value.
    assert(AttachRef < ClearValues.size());
    const float *Clear = ClearValues[AttachRef].color.float32;
    uint8_t Pixel[4] = {(uint8_t)std::round(Clear[0] * 255),
                        (uint8_t)std::round(Clear[1] * 255),
                        (uint8_t)std::round(Clear[2] * 255),
                        (uint8_t)std::round(Clear[3] * 255)};

    // Store clear value to each pixel in attachment.
    for (uint32_t y = 0; y < FB.getHeight(); y++)
    {
      for (uint32_t x = 0; x < FB.getWidth(); x++)
      {
        uint64_t Address = FB.getAttachments()[AttachIndex];
        Address += (x + (y * FB.getWidth())) * 4;
        FB.getDevice().getGlobalMemory().store(Address, 4, Pixel);
      }
    }
  }
}

void RenderPassInstance::end()
{
  assert(Rendering);

  endSubpass();
  Rendering = false;
}

void RenderPassInstance::endSubpass()
{
  // TODO: Perform multisample resolve operations if necessary
}

void RenderPassInstance::nextSubpass()
{
  assert(Rendering);

  endSubpass();
  ++SubpassIndex;
  beginSubpass();
}

} // namespace talvos
