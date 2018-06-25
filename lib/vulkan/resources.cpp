// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

#include "talvos/FormatUtils.h"

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
  for (uint32_t i = 0; i < bindInfoCount; i++)
  {
    // Walk through extensions.
    const void *Ext = pBindInfos[i].pNext;
    while (Ext)
    {
      switch (*(VkStructureType *)Ext)
      {
      case VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_DEVICE_GROUP_INFO:
      {
        VkBindBufferMemoryDeviceGroupInfo *BindInfo =
            (VkBindBufferMemoryDeviceGroupInfo *)Ext;
        assert(BindInfo->deviceIndexCount == 0);
        break;
      }
      default:
        assert(false && "Unimplemented extension");
      }

      Ext = ((void **)Ext)[1];
    }

    vkBindBufferMemory(device, pBindInfos[i].buffer, pBindInfos[i].memory,
                       pBindInfos[i].memoryOffset);
  }
  return VK_SUCCESS;
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
    switch (*(VkStructureType *)Ext)
    {
    case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS:
    {
      VkMemoryDedicatedRequirements *Requirements =
          (VkMemoryDedicatedRequirements *)Ext;
      Requirements->prefersDedicatedAllocation = VK_FALSE;
      Requirements->requiresDedicatedAllocation = VK_FALSE;
      break;
    }
    default:
      assert(false && "Unimplemented extension");
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
  pMemoryRequirements->size = pixels * talvos::getElementSize(image->Format);
  pMemoryRequirements->alignment = 1;
  pMemoryRequirements->memoryTypeBits = 0b1;
}

VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements2(
    VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo,
    VkMemoryRequirements2 *pMemoryRequirements)
{
  vkGetImageMemoryRequirements(device, pInfo->image,
                               &pMemoryRequirements->memoryRequirements);

  {
    // Walk through image requirement extensions.
    const void *Ext = pInfo->pNext;
    while (Ext)
    {
      switch (*(VkStructureType *)Ext)
      {
      default:
        assert(false && "Unimplemented extension");
      }

      Ext = ((void **)Ext)[1];
    }
  }

  {
    // Walk through memory requirement extensions.
    void *Ext = pMemoryRequirements->pNext;
    while (Ext)
    {
      switch (*(VkStructureType *)Ext)
      {
      case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS:
      {
        VkMemoryDedicatedRequirements *Requirements =
            (VkMemoryDedicatedRequirements *)Ext;
        Requirements->prefersDedicatedAllocation = VK_FALSE;
        Requirements->requiresDedicatedAllocation = VK_FALSE;
        break;
      }
      default:
        assert(false && "Unimplemented extension");
      }

      Ext = ((void **)Ext)[1];
    }
  }
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
