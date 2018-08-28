#include "common.h"

#include <cstdlib>
#include <iostream>

// TODO: Refactor more boilerplate code into common.cpp and TestContext.

int main(int argc, char *argv[])
{
  VkResult Result;
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
  VkDescriptorSet DescriptorSet;
  VkCommandBuffer CommandBuffer;
  uint32_t *ShaderCode = NULL;
  uint32_t *HostA;
  uint32_t *HostB;
  uint32_t *HostC;

  unsigned N = 65536;

  // Create test context.
  TestContext Context("test/vecadd");

  // Find a host-visible memory.
  VkMemoryAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                       NULL, N * sizeof(uint32_t), UINT32_MAX};
  VkPhysicalDeviceMemoryProperties MemProperties;
  vkGetPhysicalDeviceMemoryProperties(Context.PhysicalDevice, &MemProperties);
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
    std::cerr << "Failed to find host visible memory type." << std::endl;
    exit(1);
  }

  // Allocate memory for each device buffer.
  Result = vkAllocateMemory(Context.Device, &AllocateInfo, NULL, &MemA);
  check(Result, "allocating MemA");
  Result = vkAllocateMemory(Context.Device, &AllocateInfo, NULL, &MemB);
  check(Result, "allocating MemB");
  Result = vkAllocateMemory(Context.Device, &AllocateInfo, NULL, &MemC);
  check(Result, "allocating MemC");

  // Initialize device memory.
  Result = vkMapMemory(Context.Device, MemA, 0, N * sizeof(uint32_t), 0,
                       (void **)&HostA);
  check(Result, "mapping MemA");
  Result = vkMapMemory(Context.Device, MemB, 0, N * sizeof(uint32_t), 0,
                       (void **)&HostB);
  check(Result, "mapping MemB");
  Result = vkMapMemory(Context.Device, MemC, 0, N * sizeof(uint32_t), 0,
                       (void **)&HostC);
  check(Result, "mapping MemC");
  for (unsigned i = 0; i < N; i++)
  {
    HostA[i] = i;
    HostB[i] = 42 + i * 3;
    HostC[i] = 0;
  }
  vkUnmapMemory(Context.Device, MemA);
  vkUnmapMemory(Context.Device, MemB);
  vkUnmapMemory(Context.Device, MemC);

  // Create buffers.
  VkBufferCreateInfo BufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                         NULL,
                                         0,
                                         N * sizeof(uint32_t),
                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                         VK_SHARING_MODE_EXCLUSIVE,
                                         0,
                                         NULL};
  Result = vkCreateBuffer(Context.Device, &BufferCreateInfo, NULL, &BufA);
  check(Result, "creating BufA");
  Result = vkCreateBuffer(Context.Device, &BufferCreateInfo, NULL, &BufB);
  check(Result, "creating BufB");
  Result = vkCreateBuffer(Context.Device, &BufferCreateInfo, NULL, &BufC);
  check(Result, "creating BufC");

  // Bind memory to buffers.
  Result = vkBindBufferMemory(Context.Device, BufA, MemA, 0);
  check(Result, "binding BufA");
  Result = vkBindBufferMemory(Context.Device, BufB, MemB, 0);
  check(Result, "binding BufB");
  Result = vkBindBufferMemory(Context.Device, BufC, MemC, 0);
  check(Result, "binding BufC");

  // Load SPIR-V shader code.
  size_t ShaderCodeSize = 0;
  FILE *CodeFile = fopen("../misc/vecadd.spv", "rb");
  if (!CodeFile)
  {
    std::cerr << "Failed to open SPIR-V file." << std::endl;
    exit(1);
  }
  fseek(CodeFile, 0, SEEK_END);
  ShaderCodeSize = ftell(CodeFile);
  ShaderCode = (uint32_t *)malloc(ShaderCodeSize);
  fseek(CodeFile, 0, SEEK_SET);
  fread(ShaderCode, 1, ShaderCodeSize, CodeFile);
  fclose(CodeFile);

  // Create shader module.
  VkShaderModuleCreateInfo ModuleCreateInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, NULL, 0, ShaderCodeSize,
      ShaderCode};
  Result =
      vkCreateShaderModule(Context.Device, &ModuleCreateInfo, NULL, &Module);
  check(Result, "creating shader module");

  // Create descriptor set layout.
  VkDescriptorSetLayoutBinding DescriptorSetLayoutBindings[3] = {
      {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
       NULL},
      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
       NULL},
      {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
       NULL}};
  VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, NULL, 0, 3,
      DescriptorSetLayoutBindings};
  Result = vkCreateDescriptorSetLayout(Context.Device,
                                       &DescriptorSetLayoutCreateInfo, NULL,
                                       &DescriptorSetLayout);
  check(Result, "creating descriptor set layout");

  // Create shader pipeline layout.
  VkPipelineShaderStageCreateInfo ShaderStageCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      NULL,
      0,
      VK_SHADER_STAGE_COMPUTE_BIT,
      Module,
      "vecadd",
      NULL};
  VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      NULL,
      0,
      1,
      &DescriptorSetLayout,
      0,
      NULL};
  Result = vkCreatePipelineLayout(Context.Device, &PipelineLayoutCreateInfo,
                                  NULL, &PipelineLayout);
  check(Result, "creating pipeline layout");

  // Create compute pipeline.
  VkComputePipelineCreateInfo PipelineCreateInfo = {
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      NULL,
      0,
      ShaderStageCreateInfo,
      PipelineLayout,
      NULL,
      0};
  Result = vkCreateComputePipelines(Context.Device, VK_NULL_HANDLE, 1,
                                    &PipelineCreateInfo, NULL, &Pipeline);
  check(Result, "creating compute pipeline");

  // Allocate descriptor set.
  VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, NULL,
      Context.DescriptorPool, 1, &DescriptorSetLayout};
  Result = vkAllocateDescriptorSets(Context.Device, &DescriptorSetAllocateInfo,
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
      NULL,
      DescriptorSet,
      0,
      0,
      3,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      NULL,
      DescriptorBufferInfo,
      NULL};
  vkUpdateDescriptorSets(Context.Device, 1, &DescriptorWrites, 0, NULL);

  // Allocate command buffer.
  VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, NULL, Context.CommandPool,
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1};
  Result = vkAllocateCommandBuffers(Context.Device, &CommandBufferAllocateInfo,
                                    &CommandBuffer);
  check(Result, "creating command buffer");

  // Begin recording commands.
  VkCommandBufferBeginInfo BeginInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL,
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, NULL};
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

  // Submit command buffer to queue.
  VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO,
                             NULL,
                             0,
                             NULL,
                             NULL,
                             1,
                             &CommandBuffer,
                             0,
                             NULL};
  Result = vkQueueSubmit(Context.Queue, 1, &SubmitInfo, VK_NULL_HANDLE);
  check(Result, "submitting command");

  // Wait until commands have completed.
  Result = vkQueueWaitIdle(Context.Queue);
  check(Result, "waiting for queue to be idle");

  // Check results.
  Result = vkMapMemory(Context.Device, MemA, 0, N * sizeof(uint32_t), 0,
                       (void **)&HostA);
  check(Result, "mapping MemA");
  Result = vkMapMemory(Context.Device, MemB, 0, N * sizeof(uint32_t), 0,
                       (void **)&HostB);
  check(Result, "mapping MemB");
  Result = vkMapMemory(Context.Device, MemC, 0, N * sizeof(uint32_t), 0,
                       (void **)&HostC);
  check(Result, "mapping MemC");
  for (unsigned i = 0; i < N; i++)
  {
    if (HostC[i] != (HostA[i] + HostB[i]))
    {
      std::cout << "Error at index " << i << ": " << HostC[i] << " != ("
                << HostA[i] << " + " << HostB[i] << ")" << std::endl;
      exit(1);
    }
  }
  std::cout << "All results validated correctly." << std::endl;
  vkUnmapMemory(Context.Device, MemA);
  vkUnmapMemory(Context.Device, MemB);
  vkUnmapMemory(Context.Device, MemC);

  // Cleanup.
  vkFreeCommandBuffers(Context.Device, Context.CommandPool, 1, &CommandBuffer);
  vkFreeDescriptorSets(Context.Device, Context.DescriptorPool, 1,
                       &DescriptorSet);
  vkDestroyPipeline(Context.Device, Pipeline, NULL);
  vkDestroyPipelineLayout(Context.Device, PipelineLayout, NULL);
  vkDestroyDescriptorSetLayout(Context.Device, DescriptorSetLayout, NULL);
  vkDestroyShaderModule(Context.Device, Module, NULL);
  vkDestroyBuffer(Context.Device, BufA, NULL);
  vkDestroyBuffer(Context.Device, BufB, NULL);
  vkDestroyBuffer(Context.Device, BufC, NULL);
  vkFreeMemory(Context.Device, MemA, NULL);
  vkFreeMemory(Context.Device, MemB, NULL);
  vkFreeMemory(Context.Device, MemC, NULL);

  free(ShaderCode);

  return 0;
}
