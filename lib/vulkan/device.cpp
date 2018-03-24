// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

#include <cstring>

#include "talvos/Device.h"

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
    VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkDevice *pDevice)
{
  *pDevice = new VkDevice_T;
  (*pDevice)->Device = new talvos::Device;
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator)
{
  delete device->Device;
  delete device;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDeviceGroups(
    VkInstance instance, uint32_t *pPhysicalDeviceGroupCount,
    VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDeviceGroupsKHR(
    VkInstance instance, uint32_t *pPhysicalDeviceGroupCount,
    VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount,
                           VkPhysicalDevice *pPhysicalDevices)
{
  if (pPhysicalDevices && *pPhysicalDeviceCount < 1)
    return VK_INCOMPLETE;

  *pPhysicalDeviceCount = 1;
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(VkDevice device,
                                            uint32_t queueFamilyIndex,
                                            uint32_t queueIndex,
                                            VkQueue *pQueue)
{
  *pQueue = new VkQueue_T;
  (*pQueue)->Device = device;
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue2(
    VkDevice device, const VkDeviceQueueInfo2 *pQueueInfo, VkQueue *pQueue)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties)
{
  pProperties->apiVersion = VK_API_VERSION_1_1;
  pProperties->driverVersion = 0; // TODO: Encode Talvos version somehow.
  pProperties->vendorID = 0;      // TODO: Register a vendor ID.
  pProperties->deviceID = 0;      // TODO: Something meaningful.
  pProperties->deviceType = VK_PHYSICAL_DEVICE_TYPE_OTHER;
  strcpy(pProperties->deviceName, "Talvos");

  // TODO: Populate these with real values.
  memset(&pProperties->limits, 0, sizeof(VkPhysicalDeviceLimits));
  memset(&pProperties->sparseProperties, 0,
         sizeof(VkPhysicalDeviceSparseProperties));
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2 *pProperties)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2KHR(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2 *pProperties)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
    VkQueueFamilyProperties *pQueueFamilyProperties)
{
  if (!pQueueFamilyProperties)
  {
    *pQueueFamilyPropertyCount = 1;
    return;
  }

  if (*pQueueFamilyPropertyCount < 1)
    return;

  *pQueueFamilyPropertyCount = 1;
  pQueueFamilyProperties->queueFlags =
      VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
  pQueueFamilyProperties->queueCount = 1;
  pQueueFamilyProperties->timestampValidBits = 0;
  pQueueFamilyProperties->minImageTransferGranularity = {1, 1, 1};
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties2(
    VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
    VkQueueFamilyProperties2 *pQueueFamilyProperties)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties2KHR(
    VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
    VkQueueFamilyProperties2 *pQueueFamilyProperties)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}
