// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file RenderPass.h
/// This file declares the RenderPass class and related data structures.

#ifndef TALVOS_RENDERPASS_H
#define TALVOS_RENDERPASS_H

#include <vector>

#include "vulkan/vulkan_core.h"

namespace talvos
{

/// This structure describes the attachments used by a subpass.
struct Subpass
{
  std::vector<uint32_t> InputAttachments;
  std::vector<uint32_t> ColorAttachments;
  std::vector<uint32_t> ResolveAttachments;
  std::vector<uint32_t> PreserveAttachments;
  uint32_t DepthStencilAttachment;
};

/// This class represents a Vulkan render pass.
class RenderPass
{
public:
  /// Create a render pass from sets of attachment and subpass descriptions.
  RenderPass(const std::vector<VkAttachmentDescription> &Attachments,
             std::vector<Subpass> Subpasses)
      : Attachments(Attachments), Subpasses(Subpasses){};

  /// Returns the attachment at index \p Index.
  const VkAttachmentDescription &getAttachment(uint32_t Index) const;

  /// Returns the subpass at index \p Index.
  const Subpass &getSubpass(uint32_t Index) const;

private:
  /// The attachments used by this render pass.
  std::vector<VkAttachmentDescription> Attachments;

  /// The subpasses contained in this render pass.
  const std::vector<Subpass> Subpasses;
};

}; // namespace talvos

#endif
