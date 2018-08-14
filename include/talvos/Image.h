// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Image.h
/// This file declares data structures and functions for handling images.

#ifndef TALVOS_IMAGE_H
#define TALVOS_IMAGE_H

#include "vulkan/vulkan_core.h"

namespace talvos
{

class Device;

/// This class represents an image object.
class Image
{
public:
  /// Create an image.
  Image(VkImageType Type, VkFormat Format, VkExtent3D Extent,
        uint32_t NumArrayLayers = 1, uint32_t NumMipLevels = 1)
      : Type(Type), Format(Format), Extent(Extent),
        NumArrayLayers(NumArrayLayers), NumMipLevels(NumMipLevels)
  {
    Address = 0;
  }

  /// Bind a memory address to the image (can only be called once).
  void bindAddress(uint64_t Address);

  /// Returns the memory address of the beginning of the image.
  uint64_t getAddress() const { return Address; }

  /// Returns the depth of the image.
  uint32_t getDepth() const { return Extent.depth; }

  /// Returns the depth of the image at the specified mip level.
  uint32_t getDepthAtMipLevel(uint32_t Level) const;

  /// Returns the size in bytes of a single pixel in the image.
  uint32_t getElementSize() const;

  /// Returns the size of a single layer of the image.
  VkExtent3D getExtent() const { return Extent; }

  /// Returns the format of the image.
  VkFormat getFormat() const { return Format; }

  /// Returns the height of the image.
  uint32_t getHeight() const { return Extent.height; }

  /// Returns the height of the image at the specified mip level.
  uint32_t getHeightAtMipLevel(uint32_t Level) const;

  /// Returns the offset in bytes to the beginning of the specified mip level.
  uint64_t getMipLevelOffset(uint32_t Level) const;

  /// Returns the number of array layers in the image.
  uint32_t getNumArrayLayers() const { return NumArrayLayers; }

  /// Returns the number of mip levels in the image.
  uint32_t getNumMipLevels() const { return NumMipLevels; }

  /// Returns the total size of the image (including all mip levels).
  uint64_t getTotalSize() const { return getMipLevelOffset(NumMipLevels); }

  /// Returns the type of the image.
  VkImageType getType() const { return Type; }

  /// Returns the width of the image.
  uint32_t getWidth() const { return Extent.width; }

  /// Returns the width of the image at the specified mip level.
  uint32_t getWidthAtMipLevel(uint32_t Level) const;

private:
  VkImageType Type;        ///< The image type.
  VkFormat Format;         ///< The image format.
  VkExtent3D Extent;       ///< The image extent.
  uint32_t NumArrayLayers; ///< The number of array layers.
  uint32_t NumMipLevels;   ///< The number of mip levels.

  uint64_t Address = 0; ///< The memory address of the image data.
};

/// This class represents a view into a range of image subresources.
class ImageView
{
public:
  /// Create an image view.
  ImageView(Device &Dev, const Image &Img, VkImageViewType Type,
            VkFormat Format, VkImageSubresourceRange Range);

  /// Returns the memory address of the start of the image view data.
  uint64_t getAddress() const { return Address; }

  /// Returns the base array layer of the image view.
  uint32_t getBaseArrayLayer() const { return BaseArrayLayer; }

  /// Returns the base mip level of the image view.
  uint32_t getBaseMipLevel() const { return BaseMipLevel; }

  /// Returns the format of the image.
  VkFormat getFormat() const { return Format; }

  /// Returns the image that the image view corresponds to.
  const Image &getImage() const { return Img; }

  /// Returns the number of array layers in the image view.
  uint32_t getNumArrayLayers() const { return NumArrayLayers; }

  /// Returns the number of mip levels in the image view.
  uint32_t getNumMipLevels() const { return NumMipLevels; }

  /// Returns the type of the image view.
  VkImageViewType getType() const { return Type; }

private:
  Device &Dev; ///< The device this image view is created on.

  const Image &Img; ///< The image that the image corresponds to.

  VkImageViewType Type; ///< The type of the image view.
  VkFormat Format;      ///< The format of the image view.

  uint32_t BaseArrayLayer; ///< The base array layer.
  uint32_t NumArrayLayers; ///< The number of array layers.
  uint32_t BaseMipLevel;   ///< The base mip level.
  uint32_t NumMipLevels;   ///< The number of mip levels.

  uint64_t Address; ///< The memory address of the image view data.
};

/// Returns the size in bytes for each element of an image with type \p Format.
uint32_t getElementSize(VkFormat Format);

} // namespace talvos

#endif
