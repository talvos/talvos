// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

#include "talvos/Device.h"
#include "talvos/Image.h"
#include "talvos/Memory.h"

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
  image->Image->bindAddress(memory->Address + memoryOffset);
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkBindImageMemory2(VkDevice device, uint32_t bindInfoCount,
                   const VkBindImageMemoryInfo *pBindInfos)
{
  for (uint32_t i = 0; i < bindInfoCount; i++)
  {
    // Walk through extensions.
    const void *Ext = pBindInfos[i].pNext;
    while (Ext)
    {
      switch (*(VkStructureType *)Ext)
      {
      default:
        assert(false && "Unimplemented extension");
      }

      Ext = ((void **)Ext)[1];
    }

    vkBindImageMemory(device, pBindInfos[i].image, pBindInfos[i].memory,
                      pBindInfos[i].memoryOffset);
  }
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkBindImageMemory2KHR(VkDevice device, uint32_t bindInfoCount,
                      const VkBindImageMemoryInfo *pBindInfos)
{
  return vkBindImageMemory2(device, bindInfoCount, pBindInfos);
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

  uint64_t Address = pCreateInfo->buffer->Address + pCreateInfo->offset;
  uint64_t NumBytes = pCreateInfo->range;
  if (NumBytes == VK_WHOLE_SIZE)
    NumBytes = pCreateInfo->buffer->NumBytes - pCreateInfo->offset;

  // Create image object.
  VkDeviceSize NumElements =
      NumBytes / talvos::getElementSize(pCreateInfo->format);
  VkExtent3D Extent = {(uint32_t)NumElements, 1, 1};
  (*pView)->Image = new talvos::Image(*device->Device, VK_IMAGE_TYPE_1D,
                                      pCreateInfo->format, Extent);
  (*pView)->Image->bindAddress(Address);

  // Create 1D image view object.
  VkImageSubresourceRange Range = {0, 0, 1, 0, 1};
  (*pView)->ImageView = new talvos::ImageView(
      *(*pView)->Image, VK_IMAGE_VIEW_TYPE_1D, pCreateInfo->format, Range);

  // Store image view object in memory.
  talvos::Memory &Mem = device->Device->getGlobalMemory();
  (*pView)->ObjectAddress = Mem.allocate(sizeof(talvos::ImageView *));
  Mem.store((*pView)->ObjectAddress, sizeof(talvos::ImageView *),
            (uint8_t *)&(*pView)->ImageView);

  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo,
              const VkAllocationCallbacks *pAllocator, VkImage *pImage)
{
  *pImage = new VkImage_T;
  (*pImage)->Image = new talvos::Image(
      *device->Device, pCreateInfo->imageType, pCreateInfo->format,
      pCreateInfo->extent, pCreateInfo->arrayLayers, pCreateInfo->mipLevels);

  // TODO: Handle multisampling
  assert(pCreateInfo->samples == VK_SAMPLE_COUNT_1_BIT);

  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo,
                  const VkAllocationCallbacks *pAllocator, VkImageView *pView)
{
  *pView = new VkImageView_T;
  (*pView)->ImageView =
      new talvos::ImageView(*pCreateInfo->image->Image, pCreateInfo->viewType,
                            pCreateInfo->format, pCreateInfo->subresourceRange);

  // TODO: Handle pCreateInfo->components

  // Create image view object in memory.
  talvos::Memory &Mem = device->Device->getGlobalMemory();
  (*pView)->ObjectAddress = Mem.allocate(sizeof(talvos::ImageView *));
  Mem.store((*pView)->ObjectAddress, sizeof(talvos::ImageView *),
            (uint8_t *)&(*pView)->ImageView);

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
  if (bufferView)
  {
    device->Device->getGlobalMemory().release(bufferView->ObjectAddress);
    delete bufferView->ImageView;
    delete bufferView->Image;
    delete bufferView;
  }
}

VKAPI_ATTR void VKAPI_CALL vkDestroyImage(
    VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator)
{
  if (image)
  {
    delete image->Image;
    delete image;
  }
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyImageView(VkDevice device, VkImageView imageView,
                   const VkAllocationCallbacks *pAllocator)
{
  if (imageView)
  {
    device->Device->getGlobalMemory().release(imageView->ObjectAddress);
    delete imageView->ImageView;
    delete imageView;
  }
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
  pMemoryRequirements->size = image->Image->getTotalSize();
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
  uint32_t ElementSize = image->Image->getElementSize();
  pLayout->rowPitch = image->Image->getWidth() * ElementSize;
  pLayout->depthPitch = image->Image->getHeight() * pLayout->rowPitch;
  pLayout->arrayPitch = image->Image->getDepth() * pLayout->depthPitch;
  pLayout->size = pLayout->arrayPitch;
  pLayout->offset = pSubresource->arrayLayer * pLayout->arrayPitch;
}
