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
class Device;

/// This class represents a framebuffer that can be used for rendering.
class Framebuffer
{
public:
  /// Create a framebuffer.
  Framebuffer(Device &Dev, uint32_t Width, uint32_t Height,
              const std::vector<uint64_t> &Attachments)
      : Dev(Dev), Width(Width), Height(Height), Attachments(Attachments){};

  /// Returns the list of attachment memory addresses backing this framebuffer.
  const std::vector<uint64_t> &getAttachments() const { return Attachments; }

  /// Returns the device that this framebuffer is associated with.
  Device &getDevice() const { return Dev; }

  /// Returns the height of this framebuffer in pixels.
  uint32_t getHeight() const { return Height; }

  /// Returns the width of this framebuffer in pixels.
  uint32_t getWidth() const { return Width; }

private:
  Device &Dev; ///< The device this framebuffer belongs to.

  uint32_t Width;  ///< The width of the framebuffer in pixels.
  uint32_t Height; ///< The height of the framebuffer in pixels.

  /// The memory addresses of the framebuffer attachments.
  std::vector<uint64_t> Attachments;
};

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
