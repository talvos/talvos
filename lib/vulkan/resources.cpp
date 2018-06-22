// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(VkDevice device,
                                                  VkBuffer buffer,
                                                  VkDeviceMemory memory,
                                                  VkDeviceSize memoryOffset)
{
  buffer->Address = memory->Address + memoryOffset;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkBindBufferMemory2(VkDevice device, uint32_t bindInfoCount,
                    const VkBindBufferMemoryInfo *pBindInfos)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkBindBufferMemory2KHR(VkDevice device, uint32_t bindInfoCount,
                       const VkBindBufferMemoryInfo *pBindInfos)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory(VkDevice device, VkImage image,
                                                 VkDeviceMemory memory,
                                                 VkDeviceSize memoryOffset)
{
  image->Address = memory->Address + memoryOffset;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkBindImageMemory2(VkDevice device, uint32_t bindInfoCount,
                   const VkBindImageMemoryInfo *pBindInfos)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkBindImageMemory2KHR(VkDevice device, uint32_t bindInfoCount,
                      const VkBindImageMemoryInfo *pBindInfos)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo,
               const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer)
{
  *pBuffer = new VkBuffer_T;
  (*pBuffer)->NumBytes = pCreateInfo->size;
  (*pBuffer)->Address = 0;
  (*pBuffer)->UsageFlags = pCreateInfo->usage;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo *pCreateInfo,
                   const VkAllocationCallbacks *pAllocator, VkBufferView *pView)
{
  *pView = new VkBufferView_T;
  (*pView)->Format = pCreateInfo->format;
  (*pView)->NumBytes = pCreateInfo->range;
  (*pView)->Address = pCreateInfo->buffer->Address + pCreateInfo->offset;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo,
              const VkAllocationCallbacks *pAllocator, VkImage *pImage)
{
  *pImage = new VkImage_T;
  (*pImage)->Type = pCreateInfo->imageType;
  (*pImage)->Format = pCreateInfo->format;
  (*pImage)->Extent = pCreateInfo->extent;
  (*pImage)->MipLevels = pCreateInfo->mipLevels;
  (*pImage)->ArrayLayers = pCreateInfo->arrayLayers;
  (*pImage)->Address = 0;

  // TODO: Handle multisampling
  assert(pCreateInfo->samples == VK_SAMPLE_COUNT_1_BIT);

  // TODO: Handle image layout parameter

  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo,
                  const VkAllocationCallbacks *pAllocator, VkImageView *pView)
{
  *pView = new VkImageView_T;
  (*pView)->Image = pCreateInfo->image;
  (*pView)->Type = pCreateInfo->viewType;
  (*pView)->Format = pCreateInfo->format;

  // TODO: Support other formats (just need to know bytes per pixel).
  assert(pCreateInfo->format == VK_FORMAT_R8G8B8A8_UNORM);
  VkExtent3D Extent = pCreateInfo->image->Extent;
  (*pView)->Address = pCreateInfo->image->Address +
                      pCreateInfo->subresourceRange.baseArrayLayer *
                          (Extent.width * Extent.height * 4);

  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(
    VkDevice device, VkBuffer buffer, const VkAllocationCallbacks *pAllocator)
{
  delete buffer;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyBufferView(VkDevice device, VkBufferView bufferView,
                    const VkAllocationCallbacks *pAllocator)
{
  delete bufferView;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyImage(
    VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator)
{
  delete image;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyImageView(VkDevice device, VkImageView imageView,
                   const VkAllocationCallbacks *pAllocator)
{
  delete imageView;
}

VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements(
    VkDevice device, VkBuffer buffer, VkMemoryRequirements *pMemoryRequirements)
{
  pMemoryRequirements->size = buffer->NumBytes;
  pMemoryRequirements->memoryTypeBits = 0b1;

  if (buffer->UsageFlags &
      (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
       VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT |
       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
    pMemoryRequirements->alignment = 256;
  else
    pMemoryRequirements->alignment = 1;
}

VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements2(
    VkDevice device, const VkBufferMemoryRequirementsInfo2 *pInfo,
    VkMemoryRequirements2 *pMemoryRequirements)
{
  vkGetBufferMemoryRequirements(device, pInfo->buffer,
                                &pMemoryRequirements->memoryRequirements);

  // Walk through extensions.
  void *Ext = pMemoryRequirements->pNext;
  while (Ext)
  {
    if (*(VkStructureType *)Ext ==
        VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS)
    {
      VkMemoryDedicatedRequirements *Requirements =
          (VkMemoryDedicatedRequirements *)Ext;
      Requirements->prefersDedicatedAllocation = VK_FALSE;
      Requirements->requiresDedicatedAllocation = VK_FALSE;
    }
    Ext = ((void **)Ext)[1];
  }
}

VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements2KHR(
    VkDevice device, const VkBufferMemoryRequirementsInfo2 *pInfo,
    VkMemoryRequirements2 *pMemoryRequirements)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements(
    VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements)
{
  VkDeviceSize pixels = image->Extent.width * image->Extent.height *
                        image->Extent.depth * image->ArrayLayers;
  switch (image->Format)
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
    pMemoryRequirements->size = pixels * 1;
    break;
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
    pMemoryRequirements->size = pixels * 2;
    break;
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
    pMemoryRequirements->size = pixels * 3;
    break;
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
    pMemoryRequirements->size = pixels * 4;
    break;
  case VK_FORMAT_R16G16B16_UNORM:
  case VK_FORMAT_R16G16B16_SNORM:
  case VK_FORMAT_R16G16B16_USCALED:
  case VK_FORMAT_R16G16B16_SSCALED:
  case VK_FORMAT_R16G16B16_UINT:
  case VK_FORMAT_R16G16B16_SINT:
  case VK_FORMAT_R16G16B16_SFLOAT:
    pMemoryRequirements->size = pixels * 6;
    break;
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
    pMemoryRequirements->size = pixels * 8;
    break;
  case VK_FORMAT_R32G32B32_UINT:
  case VK_FORMAT_R32G32B32_SINT:
  case VK_FORMAT_R32G32B32_SFLOAT:
    pMemoryRequirements->size = pixels * 12;
    break;
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
    pMemoryRequirements->size = pixels * 16;
    break;
  case VK_FORMAT_R64G64B64_UINT:
  case VK_FORMAT_R64G64B64_SINT:
  case VK_FORMAT_R64G64B64_SFLOAT:
    pMemoryRequirements->size = pixels * 24;
    break;
  case VK_FORMAT_R64G64B64A64_UINT:
  case VK_FORMAT_R64G64B64A64_SINT:
  case VK_FORMAT_R64G64B64A64_SFLOAT:
    pMemoryRequirements->size = pixels * 32;
    break;
  default:
    assert(false && "Unhandled image format in vkGetImageMemoryRequirements");
  }

  pMemoryRequirements->alignment = 1;
  pMemoryRequirements->memoryTypeBits = 0b1;
}

VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements2(
    VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo,
    VkMemoryRequirements2 *pMemoryRequirements)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements2KHR(
    VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo,
    VkMemoryRequirements2 *pMemoryRequirements)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkGetImageSubresourceLayout(
    VkDevice device, VkImage image, const VkImageSubresource *pSubresource,
    VkSubresourceLayout *pLayout)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}
