#include "vulkan/vk_platform.h"
#include "vulkan/vulkan_core.h"

#include <stdio.h>
#include <stdlib.h>

// TODO: Refactor the boilerplate code into a common file.

void check(VkResult Result, const char *Operation)
{
  if (Result != VK_SUCCESS)
  {
    fprintf(stderr, "Failed during operation '%s': %d\n", Operation, Result);
    exit(1);
  }
}

int main(int argc, char *argv[])
{
  VkResult Result;
  VkInstance Instance;
  VkPhysicalDevice PhysicalDevice;
  VkDevice Device;
  VkDeviceMemory MemA;
  VkDeviceMemory MemB;
  VkDeviceMemory MemC;
  VkBuffer BufA;
  VkBuffer BufB;
  VkBuffer BufC;
  VkShaderModule Module;
  VkDescriptorSetLayout DescriptorSetLayout;
  VkPipelineLayout PipelineLayout;
  VkPipeline Pipeline;
  VkDescriptorPool DescriptorPool;
  VkDescriptorSet DescriptorSet;
  VkCommandPool CommandPool;
  VkCommandBuffer CommandBuffer;
  uint32_t *ShaderCode = NULL;
  uint32_t *HostA;
  uint32_t *HostB;
  uint32_t *HostC;

  unsigned N = 65536;

  // Create instance.
  VkApplicationInfo AppInfo = {
    VK_STRUCTURE_TYPE_APPLICATION_INFO,
    NULL, "test/vecadd", 0, NULL, 0, VK_API_VERSION_1_0
  };
  VkInstanceCreateInfo InstanceCreateInfo = {
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    NULL, 0, &AppInfo, 0, NULL, 0, NULL
  };
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
    fprintf(stderr, "Failed to find queue supporting compute.\n");
    exit(1);
  }

  // Create logical device.
  float QueuePriority = 1.f;
  VkDeviceQueueCreateInfo QueueCreateInfo = {
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    NULL, 0, QueueFamilyIndex, 1, &QueuePriority
  };
  VkDeviceCreateInfo DeviceCreateInfo = {
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    NULL, 0, 1, &QueueCreateInfo, 0, NULL, 0, NULL, NULL
  };
  Result = vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, NULL, &Device);
  check(Result, "creating device");

  // Find a host-visible memory.
  VkMemoryAllocateInfo AllocateInfo = {
    VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    NULL, N*sizeof(uint32_t), UINT32_MAX
  };
  VkPhysicalDeviceMemoryProperties MemProperties;
  vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemProperties);
  for (uint32_t i = 0; i < MemProperties.memoryTypeCount; i++)
  {
    if (MemProperties.memoryTypes[i].propertyFlags &
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
      AllocateInfo.memoryTypeIndex = i;
      break;
    }
  }
  if (AllocateInfo.memoryTypeIndex == UINT32_MAX)
  {
    fprintf(stderr, "Failed to find host visible memory type.\n");
    exit(1);
  }

  // Allocate memory for each device buffer.
  Result = vkAllocateMemory(Device, &AllocateInfo, NULL, &MemA);
  check(Result, "allocating MemA");
  Result = vkAllocateMemory(Device, &AllocateInfo, NULL, &MemB);
  check(Result, "allocating MemB");
  Result = vkAllocateMemory(Device, &AllocateInfo, NULL, &MemC);
  check(Result, "allocating MemC");

  // Initialize device memory.
  Result = vkMapMemory(Device, MemA, 0, N*sizeof(uint32_t), 0, (void**)&HostA);
  check(Result, "mapping MemA");
  Result = vkMapMemory(Device, MemB, 0, N*sizeof(uint32_t), 0, (void**)&HostB);
  check(Result, "mapping MemB");
  Result = vkMapMemory(Device, MemC, 0, N*sizeof(uint32_t), 0, (void**)&HostC);
  check(Result, "mapping MemC");
  for (unsigned i = 0; i < N; i++)
  {
    HostA[i] = i;
    HostB[i] = 42 + i*3;
    HostC[i] = 0;
  }
  vkUnmapMemory(Device, MemA);
  vkUnmapMemory(Device, MemB);
  vkUnmapMemory(Device, MemC);

  // Create buffers.
  VkBufferCreateInfo BufferCreateInfo = {
    VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    NULL, 0, N*sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    VK_SHARING_MODE_EXCLUSIVE, 0, NULL
  };
  Result = vkCreateBuffer(Device, &BufferCreateInfo, NULL, &BufA);
  check(Result, "creating BufA");
  Result = vkCreateBuffer(Device, &BufferCreateInfo, NULL, &BufB);
  check(Result, "creating BufB");
  Result = vkCreateBuffer(Device, &BufferCreateInfo, NULL, &BufC);
  check(Result, "creating BufC");

  // Bind memory to buffers.
  Result = vkBindBufferMemory(Device, BufA, MemA, 0);
  check(Result, "binding BufA");
  Result = vkBindBufferMemory(Device, BufB, MemB, 0);
  check(Result, "binding BufB");
  Result = vkBindBufferMemory(Device, BufC, MemC, 0);
  check(Result, "binding BufC");

  // Load SPIR-V shader code.
  size_t ShaderCodeSize = 0;
  FILE *CodeFile = fopen("../misc/vecadd.spv", "rb");
  if (!CodeFile)
  {
    fprintf(stderr, "Failed to open SPIR-V file.\n");
    exit(1);
  }
  fseek(CodeFile, 0, SEEK_END);
  ShaderCodeSize = ftell(CodeFile);
  ShaderCode = (uint32_t*)malloc(ShaderCodeSize);
  fseek(CodeFile, 0, SEEK_SET);
  fread(ShaderCode, 1, ShaderCodeSize, CodeFile);
  fclose(CodeFile);

  // Create shader module.
  VkShaderModuleCreateInfo ModuleCreateInfo = {
    VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    NULL, 0, ShaderCodeSize, ShaderCode
  };
  Result = vkCreateShaderModule(Device, &ModuleCreateInfo, NULL, &Module);
  check(Result, "creating shader module");

  // Create descriptor set layout.
  VkDescriptorSetLayoutBinding DescriptorSetLayoutBindings[3] = {
  {
    0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL
  },
  {
    1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL
  },
  {
    2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL
  }
  };
  VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    NULL, 0, 3, DescriptorSetLayoutBindings
  };
  Result = vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo,
                                       NULL, &DescriptorSetLayout);
  check(Result, "creating descriptor set layout");

  // Create shader pipeline layout.
  VkPipelineShaderStageCreateInfo ShaderStageCreateInfo = {
    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    NULL, 0, VK_SHADER_STAGE_COMPUTE_BIT, Module, "vecadd", NULL
  };
  VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {
    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    NULL, 0, 1, &DescriptorSetLayout, 0, NULL
  };
  Result = vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, NULL,
                                  &PipelineLayout);
  check(Result, "creating pipeline layout");

  // Create compute pipeline.
  VkComputePipelineCreateInfo PipelineCreateInfo = {
    VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    NULL, 0, ShaderStageCreateInfo, PipelineLayout, NULL, 0
  };
  Result = vkCreateComputePipelines(Device, VK_NULL_HANDLE, 1,
                                    &PipelineCreateInfo, NULL, &Pipeline);
  check(Result, "creating compute pipeline");

  // Create descriptor pool.
  VkDescriptorPoolSize PoolSize = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 };
  VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    NULL, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1, 1, &PoolSize
  };
  Result = vkCreateDescriptorPool(Device, &DescriptorPoolCreateInfo, NULL,
                                  &DescriptorPool);
  check(Result, "creating descriptor pool");

  // Allocate descriptor set.
  VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    NULL, DescriptorPool, 1, &DescriptorSetLayout
  };
  Result = vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo,
                                    &DescriptorSet);
  check(Result, "allocating descriptor set");

  // Write buffer memory addresses to descriptor set.
  VkDescriptorBufferInfo DescriptorBufferInfo[3] = {
    {BufA, 0, VK_WHOLE_SIZE},
    {BufB, 0, VK_WHOLE_SIZE},
    {BufC, 0, VK_WHOLE_SIZE},
  };
  VkWriteDescriptorSet DescriptorWrites = {
    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    NULL, DescriptorSet, 0, 0, 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    NULL, DescriptorBufferInfo, NULL
  };
  vkUpdateDescriptorSets(Device, 1, &DescriptorWrites, 0, NULL);

  // Create command pool.
  VkCommandPoolCreateInfo CommandPoolCreateInfo = {
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    NULL, 0, QueueFamilyIndex
  };
  Result = vkCreateCommandPool(Device, &CommandPoolCreateInfo, NULL,
                               &CommandPool);
  check(Result, "creating command pool");

  // Allocate command buffer.
  VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    NULL, CommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1
  };
  Result = vkAllocateCommandBuffers(Device, &CommandBufferAllocateInfo,
                                    &CommandBuffer);
  check(Result, "creating command buffer");

  // Begin recording commands.
  VkCommandBufferBeginInfo BeginInfo = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    NULL, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, NULL
  };
  Result = vkBeginCommandBuffer(CommandBuffer, &BeginInfo);
  check(Result, "begin command buffer");

  // Bind compute pipeline and descriptor set.
  vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline);
  vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          PipelineLayout, 0, 1, &DescriptorSet, 0, NULL);

  // Dispatch N workgroups.
  vkCmdDispatch(CommandBuffer, N, 1, 1);

  // Finish recording commands.
  Result = vkEndCommandBuffer(CommandBuffer);
  check(Result, "end command buffer");

  // Get a device queue.
  VkQueue Queue;
  vkGetDeviceQueue(Device, QueueFamilyIndex, 0, &Queue);

  // Submit command buffer to queue.
  VkSubmitInfo SubmitInfo = {
    VK_STRUCTURE_TYPE_SUBMIT_INFO,
    NULL, 0, NULL, NULL, 1, &CommandBuffer, 0, NULL
  };
  Result = vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE);
  check(Result, "submitting command");

  // Wait until commands have completed.
  Result = vkQueueWaitIdle(Queue);
  check(Result, "waiting for queue to be idle");

  // Check results.
  Result = vkMapMemory(Device, MemA, 0, N*sizeof(uint32_t), 0, (void**)&HostA);
  check(Result, "mapping MemA");
  Result = vkMapMemory(Device, MemB, 0, N*sizeof(uint32_t), 0, (void**)&HostB);
  check(Result, "mapping MemB");
  Result = vkMapMemory(Device, MemC, 0, N*sizeof(uint32_t), 0, (void**)&HostC);
  check(Result, "mapping MemC");
  for (unsigned i = 0; i < N; i++)
  {
    if (HostC[i] != (HostA[i] + HostB[i]))
    {
      printf("Error at index %d: %d != (%d + %d)\n",
             i, HostC[i], HostA[i], HostB[i]);
      exit(1);
    }
  }
  printf("All results validated correctly.\n");
  vkUnmapMemory(Device, MemA);
  vkUnmapMemory(Device, MemB);
  vkUnmapMemory(Device, MemC);

  // Cleanup.
  vkFreeCommandBuffers(Device, CommandPool, 1, &CommandBuffer);
  vkDestroyCommandPool(Device, CommandPool, NULL);
  vkFreeDescriptorSets(Device, DescriptorPool, 1, &DescriptorSet);
  vkDestroyDescriptorPool(Device, DescriptorPool, NULL);
  vkDestroyPipeline(Device, Pipeline, NULL);
  vkDestroyPipelineLayout(Device, PipelineLayout, NULL);
  vkDestroyDescriptorSetLayout(Device, DescriptorSetLayout, NULL);
  vkDestroyShaderModule(Device, Module, NULL);
  vkDestroyBuffer(Device, BufA, NULL);
  vkDestroyBuffer(Device, BufB, NULL);
  vkDestroyBuffer(Device, BufC, NULL);
  vkFreeMemory(Device, MemA, NULL);
  vkFreeMemory(Device, MemB, NULL);
  vkFreeMemory(Device, MemC, NULL);
  vkDestroyDevice(Device, NULL);
  vkDestroyInstance(Instance, NULL);
  free(ShaderCode);

  return 0;
}
