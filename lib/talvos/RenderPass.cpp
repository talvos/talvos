// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file RenderPass.cpp
/// This file defines the RenderPass class and related data structures.

#include <cassert>

#include "talvos/Image.h"
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

    const VkAttachmentDescription &AttachDesc = RP.getAttachment(AttachRef);
    AttachmentsInitialized[AttachRef] = true;

    // Skip attachments if not clearing them.
    if (AttachDesc.loadOp != VK_ATTACHMENT_LOAD_OP_CLEAR)
      continue;

    // Generate integer clear value.
    assert(AttachRef < ClearValues.size());

    // Store clear value to each pixel in attachment.
    VkClearColorValue Color = ClearValues[AttachRef].color;
    const ImageView *Attach = FB.getAttachments()[AttachRef];
    for (uint32_t Y = 0; Y < FB.getHeight(); Y++)
    {
      for (uint32_t X = 0; X < FB.getWidth(); X++)
      {
        Attach->write(Color, X, Y);
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
