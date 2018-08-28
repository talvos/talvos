#include "common.h"

#include <cstdlib>
#include <iostream>

void check(VkResult Result, const char *Operation)
{
  if (Result != VK_SUCCESS)
  {
    std::cerr << "Failed during operation '" << Operation << "': " << Result
              << std::endl;
    exit(1);
  }
}

TestContext::TestContext(const char *AppName)
{
  VkResult Result;

  // Create instance.
  VkApplicationInfo AppInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO,
                               NULL,
                               AppName,
                               0,
                               NULL,
                               0,
                               VK_API_VERSION_1_0};
  VkInstanceCreateInfo InstanceCreateInfo = {
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      NULL,
      0,
      &AppInfo,
      0,
      NULL,
      0,
      NULL};
  Result = vkCreateInstance(&InstanceCreateInfo, NULL, &Instance);
  check(Result, "creating instance");

  // Enumerate physical devices.
  uint32_t NumDevices = 1;
  Result = vkEnumeratePhysicalDevices(Instance, &NumDevices, &PhysicalDevice);
  check(Result, "getting physical devices");

  // Find a queue family that supports compute.
  uint32_t QueueFamilyIndex = UINT32_MAX;
  uint32_t NumQueueFamilies = 10;
  VkQueueFamilyProperties QueueFamilyProperties[10];
  vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &NumQueueFamilies,
                                           QueueFamilyProperties);
  for (uint32_t i = 0; i < NumQueueFamilies; i++)
  {
    if (QueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
      QueueFamilyIndex = i;
      break;
    }
  }
  if (QueueFamilyIndex == UINT32_MAX)
  {
    std::cerr << "Failed to find queue supporting compute." << std::endl;
    exit(1);
  }

  // Create logical device.
  float QueuePriority = 1.f;
  VkDeviceQueueCreateInfo QueueCreateInfo = {
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      NULL,
      0,
      QueueFamilyIndex,
      1,
      &QueuePriority};
  VkDeviceCreateInfo DeviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                         NULL,
                                         0,
                                         1,
                                         &QueueCreateInfo,
                                         0,
                                         NULL,
                                         0,
                                         NULL,
                                         NULL};
  Result = vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, NULL, &Device);
  check(Result, "creating device");

  // Get a device queue.
  vkGetDeviceQueue(Device, QueueFamilyIndex, 0, &Queue);

  // Create descriptor pool.
  VkDescriptorPoolSize PoolSize = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3};
  VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      NULL,
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      1,
      1,
      &PoolSize};
  Result = vkCreateDescriptorPool(Device, &DescriptorPoolCreateInfo, NULL,
                                  &DescriptorPool);
  check(Result, "creating descriptor pool");

  // Create command pool.
  VkCommandPoolCreateInfo CommandPoolCreateInfo = {
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, NULL, 0, QueueFamilyIndex};
  Result =
      vkCreateCommandPool(Device, &CommandPoolCreateInfo, NULL, &CommandPool);
  check(Result, "creating command pool");
}

TestContext::~TestContext()
{
  vkDestroyCommandPool(Device, CommandPool, NULL);
  vkDestroyDescriptorPool(Device, DescriptorPool, NULL);
  vkDestroyDevice(Device, NULL);
  vkDestroyInstance(Instance, NULL);
}
