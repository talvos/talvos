// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Image.cpp
/// This file provides definitions for image functionality.

#include <cassert>

#include "talvos/Device.h"
#include "talvos/Image.h"
#include "talvos/Memory.h"
#include "talvos/Object.h"

namespace talvos
{

void Image::bindAddress(uint64_t Address)
{
  assert(this->Address == 0 && "image address is already bound");
  this->Address = Address;
}

uint32_t Image::getDepthAtMipLevel(uint32_t Level) const
{
  uint32_t Ret = Extent.depth >> Level;
  return Ret ? Ret : 1;
}

uint32_t Image::getElementSize() const
{
  return talvos::getElementSize(Format);
}

uint32_t Image::getHeightAtMipLevel(uint32_t Level) const
{
  uint32_t Ret = Extent.height >> Level;
  return Ret ? Ret : 1;
}

uint64_t Image::getMipLevelOffset(uint32_t Level) const
{
  // TODO: Precompute these offsets in constructor?
  uint64_t Offset = 0;
  for (uint32_t l = 0; l < Level; l++)
  {
    Offset += getWidthAtMipLevel(l) * getHeightAtMipLevel(l) *
              getDepthAtMipLevel(l) * NumArrayLayers * getElementSize();
  }
  return Offset;
}

uint32_t Image::getWidthAtMipLevel(uint32_t Level) const
{
  uint32_t Ret = Extent.width >> Level;
  return Ret ? Ret : 1;
}

ImageView::ImageView(Device &Dev, const Image &Img, VkImageViewType Type,
                     VkFormat Format, VkImageSubresourceRange Range)
    : Dev(Dev), Img(Img), Type(Type), Format(Format)
{
  BaseArrayLayer = Range.baseArrayLayer;
  NumArrayLayers = Range.layerCount;
  if (NumMipLevels == VK_REMAINING_ARRAY_LAYERS)
    NumArrayLayers = Img.getNumArrayLayers() - BaseArrayLayer;

  BaseMipLevel = Range.baseMipLevel;
  NumMipLevels = Range.levelCount;
  if (NumMipLevels == VK_REMAINING_MIP_LEVELS)
    NumMipLevels = Img.getNumMipLevels() - BaseMipLevel;

  uint32_t LevelWidth = Img.getWidthAtMipLevel(BaseMipLevel);
  uint32_t LevelHeight = Img.getHeightAtMipLevel(BaseMipLevel);
  Address = Img.getAddress() + Img.getMipLevelOffset(BaseMipLevel) +
            (BaseArrayLayer * LevelWidth * LevelHeight * Img.getElementSize());
}

uint64_t ImageView::getTexelAddress(const Object &Coord) const
{
  uint64_t Address = this->Address;
  switch (Type)
  {
  case VK_IMAGE_VIEW_TYPE_1D:
    Address += Coord.get<uint32_t>(0) * Img.getElementSize();
    break;
  case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
  case VK_IMAGE_VIEW_TYPE_2D:
    Address += (Coord.get<uint32_t>(0) +
                Coord.get<uint32_t>(1) * Img.getWidthAtMipLevel(BaseMipLevel)) *
               Img.getElementSize();
    break;
  case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
  case VK_IMAGE_VIEW_TYPE_3D:
  case VK_IMAGE_VIEW_TYPE_CUBE:
  case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
    Address +=
        (Coord.get<uint32_t>(0) +
         ((Coord.get<uint32_t>(1) +
           Coord.get<uint32_t>(2) * Img.getHeightAtMipLevel(BaseMipLevel)) *
          Img.getWidthAtMipLevel(BaseMipLevel))) *
        Img.getElementSize();
    break;
  default:
    assert(false && "unhandled image view type");
    abort();
  }
  return Address;
}

void ImageView::read(const Object &Coord, Object &Texel) const
{
  uint64_t Address = getTexelAddress(Coord);

  // TODO: Handle other formats
  assert(Format == VK_FORMAT_R8G8B8A8_UNORM);

  // Load raw texel data.
  uint8_t Data[4];
  Dev.getGlobalMemory().load(Data, Address, 4);

  // Convert to output format.
  Texel.set<float>(Data[0] / 255.f, 0);
  Texel.set<float>(Data[1] / 255.f, 1);
  Texel.set<float>(Data[2] / 255.f, 2);
  Texel.set<float>(Data[3] / 255.f, 3);
}

void ImageView::write(const Object &Coord, const Object &Texel) const
{
  uint64_t Address = getTexelAddress(Coord);

  // TODO: Handle other formats
  assert(Format == VK_FORMAT_R8G8B8A8_UNORM);

  // Convert to storage format.
  uint8_t Data[4];
  Data[0] = Texel.get<float>(0) * 255.f;
  Data[1] = Texel.get<float>(1) * 255.f;
  Data[2] = Texel.get<float>(2) * 255.f;
  Data[3] = Texel.get<float>(3) * 255.f;

  // Write raw texel data.
  Dev.getGlobalMemory().store(Address, 4, Data);
}

uint32_t getElementSize(VkFormat Format)
{
  switch (Format)
  {
  case VK_FORMAT_R4G4_UNORM_PACK8:
  case VK_FORMAT_R8_UNORM:
  case VK_FORMAT_R8_SNORM:
  case VK_FORMAT_R8_USCALED:
  case VK_FORMAT_R8_SSCALED:
  case VK_FORMAT_R8_UINT:
  case VK_FORMAT_R8_SINT:
  case VK_FORMAT_R8_SRGB:
  case VK_FORMAT_S8_UINT:
    return 1;

  case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
  case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
  case VK_FORMAT_R5G6B5_UNORM_PACK16:
  case VK_FORMAT_B5G6R5_UNORM_PACK16:
  case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
  case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
  case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
  case VK_FORMAT_R8G8_UNORM:
  case VK_FORMAT_R8G8_SNORM:
  case VK_FORMAT_R8G8_USCALED:
  case VK_FORMAT_R8G8_SSCALED:
  case VK_FORMAT_R8G8_UINT:
  case VK_FORMAT_R8G8_SINT:
  case VK_FORMAT_R8G8_SRGB:
  case VK_FORMAT_R16_UNORM:
  case VK_FORMAT_R16_SNORM:
  case VK_FORMAT_R16_USCALED:
  case VK_FORMAT_R16_SSCALED:
  case VK_FORMAT_R16_UINT:
  case VK_FORMAT_R16_SINT:
  case VK_FORMAT_R16_SFLOAT:
  case VK_FORMAT_D16_UNORM:
  case VK_FORMAT_R10X6_UNORM_PACK16:
  case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
  case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
  case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
  case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
  case VK_FORMAT_R12X4_UNORM_PACK16:
  case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
  case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
  case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
  case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
    return 2;

  case VK_FORMAT_R8G8B8_UNORM:
  case VK_FORMAT_R8G8B8_SNORM:
  case VK_FORMAT_R8G8B8_USCALED:
  case VK_FORMAT_R8G8B8_SSCALED:
  case VK_FORMAT_R8G8B8_UINT:
  case VK_FORMAT_R8G8B8_SINT:
  case VK_FORMAT_R8G8B8_SRGB:
  case VK_FORMAT_B8G8R8_UNORM:
  case VK_FORMAT_B8G8R8_SNORM:
  case VK_FORMAT_B8G8R8_USCALED:
  case VK_FORMAT_B8G8R8_SSCALED:
  case VK_FORMAT_B8G8R8_UINT:
  case VK_FORMAT_B8G8R8_SINT:
  case VK_FORMAT_B8G8R8_SRGB:
  case VK_FORMAT_D16_UNORM_S8_UINT:
    return 3;

  case VK_FORMAT_R8G8B8A8_UNORM:
  case VK_FORMAT_R8G8B8A8_SNORM:
  case VK_FORMAT_R8G8B8A8_USCALED:
  case VK_FORMAT_R8G8B8A8_SSCALED:
  case VK_FORMAT_R8G8B8A8_UINT:
  case VK_FORMAT_R8G8B8A8_SINT:
  case VK_FORMAT_R8G8B8A8_SRGB:
  case VK_FORMAT_B8G8R8A8_UNORM:
  case VK_FORMAT_B8G8R8A8_SNORM:
  case VK_FORMAT_B8G8R8A8_USCALED:
  case VK_FORMAT_B8G8R8A8_SSCALED:
  case VK_FORMAT_B8G8R8A8_UINT:
  case VK_FORMAT_B8G8R8A8_SINT:
  case VK_FORMAT_B8G8R8A8_SRGB:
  case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
  case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
  case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
  case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
  case VK_FORMAT_A8B8G8R8_UINT_PACK32:
  case VK_FORMAT_A8B8G8R8_SINT_PACK32:
  case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
  case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
  case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
  case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
  case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
  case VK_FORMAT_A2R10G10B10_UINT_PACK32:
  case VK_FORMAT_A2R10G10B10_SINT_PACK32:
  case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
  case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
  case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
  case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
  case VK_FORMAT_A2B10G10R10_UINT_PACK32:
  case VK_FORMAT_A2B10G10R10_SINT_PACK32:
  case VK_FORMAT_R16G16_UNORM:
  case VK_FORMAT_R16G16_SNORM:
  case VK_FORMAT_R16G16_USCALED:
  case VK_FORMAT_R16G16_SSCALED:
  case VK_FORMAT_R16G16_UINT:
  case VK_FORMAT_R16G16_SINT:
  case VK_FORMAT_R16G16_SFLOAT:
  case VK_FORMAT_R32_UINT:
  case VK_FORMAT_R32_SINT:
  case VK_FORMAT_R32_SFLOAT:
  case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
  case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
  case VK_FORMAT_X8_D24_UNORM_PACK32:
  case VK_FORMAT_D32_SFLOAT:
  case VK_FORMAT_D24_UNORM_S8_UINT:
  case VK_FORMAT_D32_SFLOAT_S8_UINT:
  case VK_FORMAT_G8B8G8R8_422_UNORM:
  case VK_FORMAT_B8G8R8G8_422_UNORM:
    return 4;

  case VK_FORMAT_R16G16B16_UNORM:
  case VK_FORMAT_R16G16B16_SNORM:
  case VK_FORMAT_R16G16B16_USCALED:
  case VK_FORMAT_R16G16B16_SSCALED:
  case VK_FORMAT_R16G16B16_UINT:
  case VK_FORMAT_R16G16B16_SINT:
  case VK_FORMAT_R16G16B16_SFLOAT:
    return 6;

  case VK_FORMAT_R16G16B16A16_UNORM:
  case VK_FORMAT_R16G16B16A16_SNORM:
  case VK_FORMAT_R16G16B16A16_USCALED:
  case VK_FORMAT_R16G16B16A16_SSCALED:
  case VK_FORMAT_R16G16B16A16_UINT:
  case VK_FORMAT_R16G16B16A16_SINT:
  case VK_FORMAT_R16G16B16A16_SFLOAT:
  case VK_FORMAT_R32G32_UINT:
  case VK_FORMAT_R32G32_SINT:
  case VK_FORMAT_R32G32_SFLOAT:
  case VK_FORMAT_R64_UINT:
  case VK_FORMAT_R64_SINT:
  case VK_FORMAT_R64_SFLOAT:
  case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
  case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
  case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
  case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
  case VK_FORMAT_BC4_UNORM_BLOCK:
  case VK_FORMAT_BC4_SNORM_BLOCK:
  case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
  case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
  case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
  case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
  case VK_FORMAT_EAC_R11_UNORM_BLOCK:
  case VK_FORMAT_EAC_R11_SNORM_BLOCK:
  case VK_FORMAT_G16B16G16R16_422_UNORM:
  case VK_FORMAT_B16G16R16G16_422_UNORM:
    return 8;

  case VK_FORMAT_R32G32B32_UINT:
  case VK_FORMAT_R32G32B32_SINT:
  case VK_FORMAT_R32G32B32_SFLOAT:
    return 12;

  case VK_FORMAT_R32G32B32A32_UINT:
  case VK_FORMAT_R32G32B32A32_SINT:
  case VK_FORMAT_R32G32B32A32_SFLOAT:
  case VK_FORMAT_R64G64_UINT:
  case VK_FORMAT_R64G64_SINT:
  case VK_FORMAT_R64G64_SFLOAT:
  case VK_FORMAT_BC2_UNORM_BLOCK:
  case VK_FORMAT_BC2_SRGB_BLOCK:
  case VK_FORMAT_BC3_UNORM_BLOCK:
  case VK_FORMAT_BC3_SRGB_BLOCK:
  case VK_FORMAT_BC5_UNORM_BLOCK:
  case VK_FORMAT_BC5_SNORM_BLOCK:
  case VK_FORMAT_BC6H_UFLOAT_BLOCK:
  case VK_FORMAT_BC6H_SFLOAT_BLOCK:
  case VK_FORMAT_BC7_UNORM_BLOCK:
  case VK_FORMAT_BC7_SRGB_BLOCK:
  case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
  case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
  case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
  case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
  case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
  case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
  case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
  case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
  case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
  case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
  case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
  case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
  case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
  case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
  case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
  case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
  case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
  case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
  case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
  case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
  case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
  case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
  case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
  case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
  case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
  case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
  case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
  case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
  case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
  case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
  case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
  case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
    return 16;

  case VK_FORMAT_R64G64B64_UINT:
  case VK_FORMAT_R64G64B64_SINT:
  case VK_FORMAT_R64G64B64_SFLOAT:
    return 24;

  case VK_FORMAT_R64G64B64A64_UINT:
  case VK_FORMAT_R64G64B64A64_SINT:
  case VK_FORMAT_R64G64B64A64_SFLOAT:
    return 32;

  default:
    assert(false && "Unrecognized image format");
    return 0;
  }
}

} // namespace talvos
