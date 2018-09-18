// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

#include <cstring>

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalBufferProperties(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo,
    VkExternalBufferProperties *pExternalBufferProperties)
{
  pExternalBufferProperties->externalMemoryProperties
      .exportFromImportedHandleTypes = 0;
  pExternalBufferProperties->externalMemoryProperties.compatibleHandleTypes = 0;
  pExternalBufferProperties->externalMemoryProperties.externalMemoryFeatures =
      0;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalBufferPropertiesKHR(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo,
    VkExternalBufferProperties *pExternalBufferProperties)
{
  vkGetPhysicalDeviceExternalBufferProperties(
      physicalDevice, pExternalBufferInfo, pExternalBufferProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalFenceProperties(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo,
    VkExternalFenceProperties *pExternalFenceProperties)
{
  pExternalFenceProperties->exportFromImportedHandleTypes = 0;
  pExternalFenceProperties->compatibleHandleTypes = 0;
  pExternalFenceProperties->externalFenceFeatures = 0;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalFencePropertiesKHR(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo,
    VkExternalFenceProperties *pExternalFenceProperties)
{
  vkGetPhysicalDeviceExternalFenceProperties(physicalDevice, pExternalFenceInfo,
                                             pExternalFenceProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalSemaphoreProperties(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo,
    VkExternalSemaphoreProperties *pExternalSemaphoreProperties)
{
  pExternalSemaphoreProperties->exportFromImportedHandleTypes = 0;
  pExternalSemaphoreProperties->compatibleHandleTypes = 0;
  pExternalSemaphoreProperties->externalSemaphoreFeatures = 0;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo,
    VkExternalSemaphoreProperties *pExternalSemaphoreProperties)
{
  vkGetPhysicalDeviceExternalSemaphoreProperties(
      physicalDevice, pExternalSemaphoreInfo, pExternalSemaphoreProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures)
{
  *pFeatures = physicalDevice->Features;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures)
{
  pFeatures->features = physicalDevice->Features;

  // Walk through list of feature extensions.
  void *Ext = pFeatures->pNext;
  while (Ext)
  {
    switch (*(VkStructureType *)Ext)
    {
    case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES:
    {
      VkPhysicalDevice16BitStorageFeatures *Features =
          (VkPhysicalDevice16BitStorageFeatures *)Ext;
      Features->storageBuffer16BitAccess = VK_FALSE;
      Features->uniformAndStorageBuffer16BitAccess = VK_FALSE;
      Features->storagePushConstant16 = VK_FALSE;
      Features->storageInputOutput16 = VK_FALSE;
      break;
    }
    case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES:
    {
      VkPhysicalDeviceMultiviewFeatures *Features =
          (VkPhysicalDeviceMultiviewFeatures *)Ext;
      Features->multiview = VK_FALSE;
      Features->multiviewGeometryShader = VK_FALSE;
      Features->multiviewTessellationShader = VK_FALSE;
      break;
    }
    case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES:
    {
      VkPhysicalDeviceProtectedMemoryFeatures *Features =
          (VkPhysicalDeviceProtectedMemoryFeatures *)Ext;
      Features->protectedMemory = VK_FALSE;
      break;
    }
    case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES:
    {
      VkPhysicalDeviceSamplerYcbcrConversionFeatures *Features =
          (VkPhysicalDeviceSamplerYcbcrConversionFeatures *)Ext;
      Features->samplerYcbcrConversion = VK_FALSE;
      break;
    }
    case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTER_FEATURES:
    {
      VkPhysicalDeviceVariablePointerFeatures *Features =
          (VkPhysicalDeviceVariablePointerFeatures *)Ext;
      Features->variablePointersStorageBuffer = VK_TRUE;
      Features->variablePointers = VK_TRUE;
      break;
    }
    default:
      break;
    }

    Ext = ((void **)Ext)[1];
  }
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2KHR(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures)
{
  vkGetPhysicalDeviceFeatures2(physicalDevice, pFeatures);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties(
    VkPhysicalDevice physicalDevice, VkFormat format,
    VkFormatProperties *pFormatProperties)
{
  switch (format)
  {
  case VK_FORMAT_R8_SNORM:
  case VK_FORMAT_R8G8_SNORM:
  case VK_FORMAT_R8G8B8_SNORM:
  case VK_FORMAT_R8G8B8A8_SNORM:
  case VK_FORMAT_R8_UNORM:
  case VK_FORMAT_R8G8_UNORM:
  case VK_FORMAT_R8G8B8_UNORM:
  case VK_FORMAT_R8G8B8A8_UNORM:
  case VK_FORMAT_R8_SINT:
  case VK_FORMAT_R8G8_SINT:
  case VK_FORMAT_R8G8B8_SINT:
  case VK_FORMAT_R8G8B8A8_SINT:
  case VK_FORMAT_R8_UINT:
  case VK_FORMAT_R8G8_UINT:
  case VK_FORMAT_R8G8B8_UINT:
  case VK_FORMAT_R8G8B8A8_UINT:
  case VK_FORMAT_R8_SSCALED:
  case VK_FORMAT_R8G8_SSCALED:
  case VK_FORMAT_R8G8B8_SSCALED:
  case VK_FORMAT_R8G8B8A8_SSCALED:
  case VK_FORMAT_R8_USCALED:
  case VK_FORMAT_R8G8_USCALED:
  case VK_FORMAT_R8G8B8_USCALED:
  case VK_FORMAT_R8G8B8A8_USCALED:
  case VK_FORMAT_B8G8R8_SNORM:
  case VK_FORMAT_B8G8R8A8_SNORM:
  case VK_FORMAT_B8G8R8_UNORM:
  case VK_FORMAT_B8G8R8A8_UNORM:
  case VK_FORMAT_B8G8R8_SINT:
  case VK_FORMAT_B8G8R8A8_SINT:
  case VK_FORMAT_B8G8R8_UINT:
  case VK_FORMAT_B8G8R8A8_UINT:
  case VK_FORMAT_B8G8R8_SSCALED:
  case VK_FORMAT_B8G8R8A8_SSCALED:
  case VK_FORMAT_B8G8R8_USCALED:
  case VK_FORMAT_B8G8R8A8_USCALED:
  case VK_FORMAT_R16_SNORM:
  case VK_FORMAT_R16G16_SNORM:
  case VK_FORMAT_R16G16B16_SNORM:
  case VK_FORMAT_R16G16B16A16_SNORM:
  case VK_FORMAT_R16_UNORM:
  case VK_FORMAT_R16G16_UNORM:
  case VK_FORMAT_R16G16B16_UNORM:
  case VK_FORMAT_R16G16B16A16_UNORM:
  case VK_FORMAT_R16_SINT:
  case VK_FORMAT_R16G16_SINT:
  case VK_FORMAT_R16G16B16_SINT:
  case VK_FORMAT_R16G16B16A16_SINT:
  case VK_FORMAT_R16_UINT:
  case VK_FORMAT_R16G16_UINT:
  case VK_FORMAT_R16G16B16_UINT:
  case VK_FORMAT_R16G16B16A16_UINT:
  case VK_FORMAT_R16_SSCALED:
  case VK_FORMAT_R16G16_SSCALED:
  case VK_FORMAT_R16G16B16_SSCALED:
  case VK_FORMAT_R16G16B16A16_SSCALED:
  case VK_FORMAT_R16_USCALED:
  case VK_FORMAT_R16G16_USCALED:
  case VK_FORMAT_R16G16B16_USCALED:
  case VK_FORMAT_R16G16B16A16_USCALED:
  case VK_FORMAT_R32_SINT:
  case VK_FORMAT_R32G32_SINT:
  case VK_FORMAT_R32G32B32_SINT:
  case VK_FORMAT_R32G32B32A32_SINT:
  case VK_FORMAT_R32_UINT:
  case VK_FORMAT_R32G32_UINT:
  case VK_FORMAT_R32G32B32_UINT:
  case VK_FORMAT_R32G32B32A32_UINT:
  case VK_FORMAT_R32_SFLOAT:
  case VK_FORMAT_R32G32_SFLOAT:
  case VK_FORMAT_R32G32B32_SFLOAT:
  case VK_FORMAT_R32G32B32A32_SFLOAT:
    pFormatProperties->linearTilingFeatures =
        pFormatProperties->optimalTilingFeatures =
            VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
            VK_FORMAT_FEATURE_TRANSFER_DST_BIT |
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
            VK_FORMAT_FEATURE_BLIT_SRC_BIT |
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
            VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT |
            VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT |
            VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
            VK_FORMAT_FEATURE_BLIT_DST_BIT |
            VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT |
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    pFormatProperties->bufferFeatures =
        VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT |
        VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT |
        VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT |
        VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT;
    break;
  default:
    pFormatProperties->linearTilingFeatures =
        pFormatProperties->optimalTilingFeatures = 0;
    pFormatProperties->bufferFeatures = 0;
    break;
  }
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties2(
    VkPhysicalDevice physicalDevice, VkFormat format,
    VkFormatProperties2 *pFormatProperties)
{
  vkGetPhysicalDeviceFormatProperties(physicalDevice, format,
                                      &pFormatProperties->formatProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties2KHR(
    VkPhysicalDevice physicalDevice, VkFormat format,
    VkFormatProperties2 *pFormatProperties)
{
  vkGetPhysicalDeviceFormatProperties2(physicalDevice, format,
                                       pFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkImageFormatProperties *pImageFormatProperties)
{
  switch (format)
  {
  case VK_FORMAT_R8_SNORM:
  case VK_FORMAT_R8G8_SNORM:
  case VK_FORMAT_R8G8B8_SNORM:
  case VK_FORMAT_R8G8B8A8_SNORM:
  case VK_FORMAT_R8_UNORM:
  case VK_FORMAT_R8G8_UNORM:
  case VK_FORMAT_R8G8B8_UNORM:
  case VK_FORMAT_R8G8B8A8_UNORM:
  case VK_FORMAT_R8_SINT:
  case VK_FORMAT_R8G8_SINT:
  case VK_FORMAT_R8G8B8_SINT:
  case VK_FORMAT_R8G8B8A8_SINT:
  case VK_FORMAT_R8_UINT:
  case VK_FORMAT_R8G8_UINT:
  case VK_FORMAT_R8G8B8_UINT:
  case VK_FORMAT_R8G8B8A8_UINT:
  case VK_FORMAT_R8_SSCALED:
  case VK_FORMAT_R8G8_SSCALED:
  case VK_FORMAT_R8G8B8_SSCALED:
  case VK_FORMAT_R8G8B8A8_SSCALED:
  case VK_FORMAT_R8_USCALED:
  case VK_FORMAT_R8G8_USCALED:
  case VK_FORMAT_R8G8B8_USCALED:
  case VK_FORMAT_R8G8B8A8_USCALED:
  case VK_FORMAT_B8G8R8_SNORM:
  case VK_FORMAT_B8G8R8A8_SNORM:
  case VK_FORMAT_B8G8R8_UNORM:
  case VK_FORMAT_B8G8R8A8_UNORM:
  case VK_FORMAT_B8G8R8_SINT:
  case VK_FORMAT_B8G8R8A8_SINT:
  case VK_FORMAT_B8G8R8_UINT:
  case VK_FORMAT_B8G8R8A8_UINT:
  case VK_FORMAT_B8G8R8_SSCALED:
  case VK_FORMAT_B8G8R8A8_SSCALED:
  case VK_FORMAT_B8G8R8_USCALED:
  case VK_FORMAT_B8G8R8A8_USCALED:
  case VK_FORMAT_R16_SNORM:
  case VK_FORMAT_R16G16_SNORM:
  case VK_FORMAT_R16G16B16_SNORM:
  case VK_FORMAT_R16G16B16A16_SNORM:
  case VK_FORMAT_R16_UNORM:
  case VK_FORMAT_R16G16_UNORM:
  case VK_FORMAT_R16G16B16_UNORM:
  case VK_FORMAT_R16G16B16A16_UNORM:
  case VK_FORMAT_R16_SINT:
  case VK_FORMAT_R16G16_SINT:
  case VK_FORMAT_R16G16B16_SINT:
  case VK_FORMAT_R16G16B16A16_SINT:
  case VK_FORMAT_R16_UINT:
  case VK_FORMAT_R16G16_UINT:
  case VK_FORMAT_R16G16B16_UINT:
  case VK_FORMAT_R16G16B16A16_UINT:
  case VK_FORMAT_R16_SSCALED:
  case VK_FORMAT_R16G16_SSCALED:
  case VK_FORMAT_R16G16B16_SSCALED:
  case VK_FORMAT_R16G16B16A16_SSCALED:
  case VK_FORMAT_R16_USCALED:
  case VK_FORMAT_R16G16_USCALED:
  case VK_FORMAT_R16G16B16_USCALED:
  case VK_FORMAT_R16G16B16A16_USCALED:
  case VK_FORMAT_R32_SINT:
  case VK_FORMAT_R32G32_SINT:
  case VK_FORMAT_R32G32B32_SINT:
  case VK_FORMAT_R32G32B32A32_SINT:
  case VK_FORMAT_R32_UINT:
  case VK_FORMAT_R32G32_UINT:
  case VK_FORMAT_R32G32B32_UINT:
  case VK_FORMAT_R32G32B32A32_UINT:
  case VK_FORMAT_R32_SFLOAT:
  case VK_FORMAT_R32G32_SFLOAT:
  case VK_FORMAT_R32G32B32_SFLOAT:
  case VK_FORMAT_R32G32B32A32_SFLOAT:
    switch (type)
    {
    case VK_IMAGE_TYPE_1D:
      pImageFormatProperties->maxExtent = {4096, 1, 1};
      pImageFormatProperties->maxArrayLayers = 256;
      break;
    case VK_IMAGE_TYPE_2D:
      pImageFormatProperties->maxExtent = {4096, 4096, 1};
      pImageFormatProperties->maxArrayLayers = 256;
      break;
    case VK_IMAGE_TYPE_3D:
      pImageFormatProperties->maxExtent = {4096, 4096, 256};
      pImageFormatProperties->maxArrayLayers = 1;
      break;
    default:
      return VK_ERROR_FORMAT_NOT_SUPPORTED;
    }
    pImageFormatProperties->maxMipLevels = 13;
    pImageFormatProperties->sampleCounts = VK_SAMPLE_COUNT_1_BIT;
    pImageFormatProperties->maxResourceSize = 1 << 31;
    return VK_SUCCESS;
  default:
    return VK_ERROR_FORMAT_NOT_SUPPORTED;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties2(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo,
    VkImageFormatProperties2 *pImageFormatProperties)
{
  return vkGetPhysicalDeviceImageFormatProperties(
      physicalDevice, pImageFormatInfo->format, pImageFormatInfo->type,
      pImageFormatInfo->tiling, pImageFormatInfo->usage,
      pImageFormatInfo->flags, &pImageFormatProperties->imageFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties2KHR(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo,
    VkImageFormatProperties2 *pImageFormatProperties)
{
  return vkGetPhysicalDeviceImageFormatProperties2(
      physicalDevice, pImageFormatInfo, pImageFormatProperties);
}
