// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file RenderPass.cpp
/// This file defines the RenderPass class and related data structures.

#include <cassert>

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

} // namespace talvos
