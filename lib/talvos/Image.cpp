// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Image.cpp
/// This file provides definitions for image functionality.

#include <algorithm>
#include <cassert>
#include <cmath>

#include "talvos/Device.h"
#include "talvos/Image.h"
#include "talvos/Memory.h"
#include "talvos/Object.h"
#include "talvos/Type.h"

namespace talvos
{

Image::Texel::Texel(const Object &Obj)
{
  assert(Obj.getType()->getScalarType()->getSize() == 4);
  assert(Obj.getType()->getElementCount() <= 4);
  memcpy(Data, Obj.getData(), Obj.getType()->getSize());
}

Image::Texel::Texel(const VkClearColorValue &ClearColor)
{
  memcpy(Data, ClearColor.float32, 16);
}

Object Image::Texel::toObject(const talvos::Type *Ty) const
{
  assert(Ty->getScalarType()->getSize() == 4);
  assert(Ty->getElementCount() <= 4);
  Object Obj(Ty);
  memcpy(Obj.getData(), Data, Ty->getSize());
  return Obj;
}

void Image::bindAddress(uint64_t Address)
{
  assert(this->Address == 0 && "image address is already bound");
  this->Address = Address;
}

uint32_t Image::getDepth(uint32_t Level) const
{
  uint32_t Ret = Extent.depth >> Level;
  return Ret ? Ret : 1;
}

uint32_t Image::getElementSize() const
{
  return talvos::getElementSize(Format);
}

uint32_t Image::getHeight(uint32_t Level) const
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
    Offset += getWidth(l) * getHeight(l) * getDepth(l) * NumArrayLayers *
              getElementSize();
  }
  return Offset;
}

uint64_t Image::getTexelAddress(uint32_t X, uint32_t Y, uint32_t Z,
                                uint32_t Layer, uint32_t MipLevel) const
{
  return Address + getMipLevelOffset(MipLevel) +
         (X + (Y + (Z + (Layer * getDepth(MipLevel))) * getHeight(MipLevel)) *
                  getWidth(MipLevel)) *
             getElementSize();
}

uint32_t Image::getWidth(uint32_t Level) const
{
  uint32_t Ret = Extent.width >> Level;
  return Ret ? Ret : 1;
}

void Image::read(Texel &T, uint64_t Address) const
{
  // TODO: Handle other formats
  assert(Format == VK_FORMAT_R8G8B8A8_UNORM);

  // Load raw texel data.
  uint8_t Data[4];
  Dev.getGlobalMemory().load(Data, Address, 4);

  // Convert to output format.
  T.set<float>(0, Data[0] / 255.f);
  T.set<float>(1, Data[1] / 255.f);
  T.set<float>(2, Data[2] / 255.f);
  T.set<float>(3, Data[3] / 255.f);
}

template <typename IntTy> void convertUNorm(const Image::Texel &T, IntTy *Data)
{
  // Clamp and normalize each component value.
  auto convert = [](float v) -> IntTy {
    if (v < 0.f)
      return 0;
    else if (v >= 1.f)
      return (IntTy)~0U;
    else
      return (uint8_t)std::round(v * (IntTy)(~0U));
  };
  Data[0] = convert(T.get<float>(0));
  Data[1] = convert(T.get<float>(1));
  Data[2] = convert(T.get<float>(2));
  Data[3] = convert(T.get<float>(3));
}

void Image::write(const Texel &T, uint64_t Address) const
{
  // Will point to texel data to be written to memory.
  const uint8_t *Data;

  // Used as intermediate buffer when conversions need to happen.
  uint8_t TData[32];

  switch (Format)
  {
  case VK_FORMAT_R32_SFLOAT:
  case VK_FORMAT_R32G32B32A32_SFLOAT:
  case VK_FORMAT_R32_UINT:
  case VK_FORMAT_R32G32B32A32_UINT:
    Data = T.getData();
    break;
  case VK_FORMAT_R8G8B8A8_UNORM:
    convertUNorm(T, TData);
    Data = TData;
    break;
  default:
    assert(false && "Unhandled format");
  }

  // Write raw texel data.
  Dev.getGlobalMemory().store(Address, getElementSize(), Data);
}

ImageView::ImageView(const Image &Img, VkImageViewType Type, VkFormat Format,
                     VkImageSubresourceRange Range)
    : Img(Img), Type(Type), Format(Format)
{
  BaseArrayLayer = Range.baseArrayLayer;
  NumArrayLayers = Range.layerCount;
  if (NumArrayLayers == VK_REMAINING_ARRAY_LAYERS)
    NumArrayLayers = Img.getNumArrayLayers() - BaseArrayLayer;

  BaseMipLevel = Range.baseMipLevel;
  NumMipLevels = Range.levelCount;
  if (NumMipLevels == VK_REMAINING_MIP_LEVELS)
    NumMipLevels = Img.getNumMipLevels() - BaseMipLevel;

  uint32_t LevelWidth = Img.getWidth(BaseMipLevel);
  uint32_t LevelHeight = Img.getHeight(BaseMipLevel);
  Address = Img.getAddress() + Img.getMipLevelOffset(BaseMipLevel) +
            (BaseArrayLayer * LevelWidth * LevelHeight * Img.getElementSize());
}

uint32_t ImageView::getDepth(uint32_t Level) const
{
  return Img.getDepth(BaseMipLevel + Level);
}

uint32_t ImageView::getHeight(uint32_t Level) const
{
  return Img.getHeight(BaseMipLevel + Level);
}

uint64_t ImageView::getTexelAddress(uint32_t X, uint32_t Y, uint32_t Z,
                                    uint32_t Layer) const
{
  return Address + (X + (Y + (Z + (Layer * Img.getDepth(BaseMipLevel))) *
                                 Img.getHeight(BaseMipLevel)) *
                            Img.getWidth(BaseMipLevel)) *
                       Img.getElementSize();
}

uint32_t ImageView::getWidth(uint32_t Level) const
{
  return Img.getWidth(BaseMipLevel + Level);
}

bool ImageView::is1D() const
{
  return Type == VK_IMAGE_VIEW_TYPE_1D || Type == VK_IMAGE_VIEW_TYPE_1D_ARRAY;
}

bool ImageView::is2D() const
{
  return Type == VK_IMAGE_VIEW_TYPE_2D || Type == VK_IMAGE_VIEW_TYPE_2D_ARRAY;
}

bool ImageView::is3D() const { return Type == VK_IMAGE_VIEW_TYPE_3D; }

bool ImageView::isCube() const
{
  return Type == VK_IMAGE_VIEW_TYPE_CUBE ||
         Type == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
}

void ImageView::read(Image::Texel &T, uint32_t X, uint32_t Y, uint32_t Z,
                     uint32_t Layer) const
{
  Img.read(T, getTexelAddress(X, Y, Z, Layer));
}

void ImageView::write(const Image::Texel &T, uint32_t X, uint32_t Y, uint32_t Z,
                      uint32_t Layer) const
{
  Img.write(T, getTexelAddress(X, Y, Z, Layer));
}

void Sampler::sample(const talvos::ImageView *Image, Image::Texel &Texel,
                     float S, float T, float R, float A, float Lod) const
{
  // TODO: Handle non-zero Lod
  assert(Lod == 0);

  // TODO: Handle other formats (with integer types)
  assert(Image->getFormat() == VK_FORMAT_R8G8B8A8_UNORM);

  // TODO: Handle anisotropic filtering
  assert(Info.anisotropyEnable == VK_FALSE);

  // TODO: Handle comparison
  assert(Info.compareEnable == VK_FALSE);

  // TODO: Handle cube maps
  assert(!Image->isCube());

  // Select array layer.
  uint32_t Layer =
      std::clamp((uint32_t)std::round(A), 0U, Image->getNumArrayLayers() - 1);

  // TODO: Handle unnormalized coordinates
  assert(Info.unnormalizedCoordinates == VK_FALSE);
  float U = S * Image->getWidth();
  float V = T * Image->getHeight();
  float W = R * Image->getDepth();

  // Compute neighboring coordinates and weights for linear filtering.
  int32_t I0 = floor(U - 0.5f);
  int32_t I1 = I0 + 1;
  int32_t J0 = floor(V - 0.5f);
  int32_t J1 = J0 + 1;
  int32_t K0 = floor(W - 0.5f);
  int32_t K1 = K0 + 1;
  float Alpha = (U - 0.5f) - I0;
  float Beta = (V - 0.5f) - J0;
  float Gamma = (W - 0.5f) - K0;

  // Handle edge behavior for coordinates.
  auto handleEdge = [](VkSamplerAddressMode AddrMode, int32_t Coord,
                       uint32_t Dim) {
    switch (AddrMode)
    {
    case VK_SAMPLER_ADDRESS_MODE_REPEAT:
      return Coord % Dim;
    case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
      return (uint32_t)std::clamp(Coord, 0, (int32_t)Dim - 1);
    default:
      // TODO: handle other addressing modes
      assert(false && "unhandled sampler addressing mode");
      return ~0U;
    }
  };
  I0 = handleEdge(Info.addressModeU, I0, Image->getWidth());
  I1 = handleEdge(Info.addressModeU, I1, Image->getWidth());
  J0 = handleEdge(Info.addressModeV, J0, Image->getHeight());
  J1 = handleEdge(Info.addressModeV, J1, Image->getHeight());
  K0 = handleEdge(Info.addressModeW, K0, Image->getDepth());
  K1 = handleEdge(Info.addressModeW, K1, Image->getDepth());

  // TODO: Select magFilter/minFilter based on Lod
  if (Info.magFilter == VK_FILTER_LINEAR)
  {
    // Load texel region for linear filtering.
    // Re-use texels when dimensionality is not 3D.
    Image::Texel T000, T100, T010, T110, T001, T101, T011, T111;
    Image->getImage().read(T000, Image->getTexelAddress(I0, J0, K0, Layer));
    Image->getImage().read(T100, Image->getTexelAddress(I1, J0, K0, Layer));
    if (Image->is1D())
    {
      T010 = T000;
      T110 = T100;
      T001 = T000;
      T101 = T100;
      T011 = T000;
      T111 = T100;
    }
    else
    {
      Image->getImage().read(T010, Image->getTexelAddress(I0, J1, K0, Layer));
      Image->getImage().read(T110, Image->getTexelAddress(I1, J1, K0, Layer));
      if (Image->is2D())
      {
        T001 = T000;
        T101 = T100;
        T011 = T010;
        T111 = T110;
      }
      else
      {
        assert(Image->is3D());
        Image->getImage().read(T001, Image->getTexelAddress(I0, J0, K1, Layer));
        Image->getImage().read(T101, Image->getTexelAddress(I1, J0, K1, Layer));
        Image->getImage().read(T011, Image->getTexelAddress(I0, J1, K1, Layer));
        Image->getImage().read(T111, Image->getTexelAddress(I1, J1, K1, Layer));
      }
    }

    // Apply linear filter to texels.
    auto linear = [&](uint32_t C) {
      return ((1 - Alpha) * (1 - Beta) * (1 - Gamma) * T000.get<float>(C)) +
             (Alpha * (1 - Beta) * (1 - Gamma) * T100.get<float>(C)) +
             ((1 - Alpha) * Beta * (1 - Gamma) * T010.get<float>(C)) +
             (Alpha * Beta * (1 - Gamma) * T110.get<float>(C)) +
             ((1 - Alpha) * (1 - Beta) * Gamma * T001.get<float>(C)) +
             (Alpha * (1 - Beta) * Gamma * T101.get<float>(C)) +
             ((1 - Alpha) * Beta * Gamma * T011.get<float>(C)) +
             (Alpha * Beta * Gamma * T111.get<float>(C));
    };
    Texel.set<float>(0, linear(0));
    Texel.set<float>(1, linear(1));
    Texel.set<float>(2, linear(2));
    Texel.set<float>(3, linear(3));
  }
  else
  {
    assert(Info.magFilter == VK_FILTER_NEAREST);

    // Select nearest coordinates based on weights.
    int32_t I = Alpha <= 0.5 ? I0 : I1;
    int32_t J = Beta <= 0.5 ? J0 : J1;
    int32_t K = Gamma <= 0.5 ? K0 : K1;

    // Load texel.
    Image::Texel T;
    Image->getImage().read(T, Image->getTexelAddress(I, J, K, Layer));
    Texel.set<float>(0, T.get<float>(0));
    Texel.set<float>(1, T.get<float>(1));
    Texel.set<float>(2, T.get<float>(2));
    Texel.set<float>(3, T.get<float>(3));
  }
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
