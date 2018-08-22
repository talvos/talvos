// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Image.h
/// This file declares data structures and functions for handling images.

#ifndef TALVOS_IMAGE_H
#define TALVOS_IMAGE_H

#include "vulkan/vulkan_core.h"

#include "talvos/Object.h"

namespace talvos
{

class Device;

/// This class represents an image object.
class Image
{
public:
  /// Abstract class for wrapping a source of texel data.
  /// Provides methods to extract texel components of various types.
  class TexelWrapper
  {
  public:
    /// Get the component at index \p C as a 32-bit floating point value.
    virtual float getFloat(uint32_t C) const = 0;

    /// Get the component at index \p C as a signed 32-bit integer value.
    virtual int32_t getSInt(uint32_t C) const = 0;

    /// Get the component at index \p C as an unsigned 32-bit integer value.
    virtual uint32_t getUInt(uint32_t C) const = 0;
  };

  /// Texel wrapper for VkClearColorValue.
  class ClearColorTexel : public TexelWrapper
  {
  public:
    ClearColorTexel(VkClearColorValue Color) : Color(Color) {}
    float getFloat(uint32_t c) const override { return Color.float32[c]; }
    int32_t getSInt(uint32_t c) const override { return Color.int32[c]; }
    uint32_t getUInt(uint32_t c) const override { return Color.uint32[c]; }

  private:
    VkClearColorValue Color;
  };

  /// Texel wrapper for talvos::Object.
  class ObjectTexel : public TexelWrapper
  {
  public:
    ObjectTexel(const Object &Obj) : Obj(Obj) {}
    float getFloat(uint32_t C) const override { return Obj.get<float>(C); }
    int32_t getSInt(uint32_t C) const override { return Obj.get<int32_t>(C); }
    uint32_t getUInt(uint32_t C) const override { return Obj.get<uint32_t>(C); }

  private:
    const Object &Obj;
  };

public:
  /// Create an image.
  Image(Device &Dev, VkImageType Type, VkFormat Format, VkExtent3D Extent,
        uint32_t NumArrayLayers = 1, uint32_t NumMipLevels = 1)
      : Dev(Dev), Type(Type), Format(Format), Extent(Extent),
        NumArrayLayers(NumArrayLayers), NumMipLevels(NumMipLevels)
  {
    Address = 0;
  }

  /// Bind a memory address to the image (can only be called once).
  void bindAddress(uint64_t Address);

  /// Returns the memory address of the beginning of the image.
  uint64_t getAddress() const { return Address; }

  /// Returns the depth of the image at the specified mip level.
  uint32_t getDepth(uint32_t Level = 0) const;

  /// Returns the size in bytes of a single pixel in the image.
  uint32_t getElementSize() const;

  /// Returns the size of a single layer of the image.
  VkExtent3D getExtent() const { return Extent; }

  /// Returns the format of the image.
  VkFormat getFormat() const { return Format; }

  /// Returns the height of the image at the specified mip level.
  uint32_t getHeight(uint32_t Level = 0) const;

  /// Returns the offset in bytes to the beginning of the specified mip level.
  uint64_t getMipLevelOffset(uint32_t Level) const;

  /// Returns the number of array layers in the image.
  uint32_t getNumArrayLayers() const { return NumArrayLayers; }

  /// Returns the number of mip levels in the image.
  uint32_t getNumMipLevels() const { return NumMipLevels; }

  /// Returns the address in memory of the texel at the specified coordinate.
  uint64_t getTexelAddress(uint32_t X, uint32_t Y = 0, uint32_t Z = 0,
                           uint32_t Layer = 0, uint32_t MipLevel = 0) const;

  /// Returns the total size of the image (including all mip levels).
  uint64_t getTotalSize() const { return getMipLevelOffset(NumMipLevels); }

  /// Returns the type of the image.
  VkImageType getType() const { return Type; }

  /// Returns the width of the image at the specified mip level.
  uint32_t getWidth(uint32_t Level = 0) const;

  /// Read a texel from the image at the specified address.
  void read(Object &Texel, uint64_t Address) const;

  /// Write a texel to the image at the specified address.
  void write(const Image::TexelWrapper &&CE, uint64_t Address) const;

private:
  Device &Dev; ///< The device this image view is created on.

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
  ImageView(const Image &Img, VkImageViewType Type, VkFormat Format,
            VkImageSubresourceRange Range);

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

  /// Returns the address in memory of the texel at the specified coordinate.
  uint64_t getTexelAddress(uint32_t X, uint32_t Y = 0, uint32_t Z = 0,
                           uint32_t Layer = 0) const;

  /// Returns the type of the image view.
  VkImageViewType getType() const { return Type; }

  /// Read a texel from the image view at the specified coordinate.
  void read(Object &Texel, uint32_t X, uint32_t Y = 0, uint32_t Z = 0,
            uint32_t Layer = 0) const;

  /// Write a texel to the image view at the specified coordinate.
  void write(const Image::TexelWrapper &&Texel, uint32_t X, uint32_t Y = 0,
             uint32_t Z = 0, uint32_t Layer = 0) const;

private:
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
