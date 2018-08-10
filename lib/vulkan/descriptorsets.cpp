// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <functional>

#include "runtime.h"

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets(
    VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo,
    VkDescriptorSet *pDescriptorSets)
{
  for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++)
  {
    pDescriptorSets[i] = new VkDescriptorSet_T;
    pDescriptorSets[i]->Layout = pAllocateInfo->pSetLayouts[i];
    pAllocateInfo->descriptorPool->Pool.insert(pDescriptorSets[i]);
  }
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets(
    VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount,
    const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount,
    const uint32_t *pDynamicOffsets)
{
  talvos::DescriptorSetMap *DSM;
  switch (pipelineBindPoint)
  {
  case VK_PIPELINE_BIND_POINT_GRAPHICS:
    DSM = &commandBuffer->DescriptorSetsGraphics;
    break;
  case VK_PIPELINE_BIND_POINT_COMPUTE:
    DSM = &commandBuffer->DescriptorSetsCompute;
    break;
  default:
    assert(false && "invalid pipeline bind point");
  }

  // Update descriptor set map.
  uint32_t OffsetIndex = 0;
  for (uint32_t i = 0; i < descriptorSetCount; i++)
  {
    (*DSM)[firstSet + i] = pDescriptorSets[i]->DescriptorSet;

    // Add dynamic offsets.
    for (auto &Mapping : (*DSM)[firstSet + i])
    {
      // Skip non-dynamic resources.
      const VkDescriptorType &Type =
          pDescriptorSets[i]->Layout->BindingTypes[Mapping.first.first];
      if (!(Type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
            Type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC))
        continue;

      assert(OffsetIndex < dynamicOffsetCount);
      Mapping.second += pDynamicOffsets[OffsetIndex];

      OffsetIndex++;
    }
  }

  assert(OffsetIndex == dynamicOffsetCount);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPushConstants(VkCommandBuffer commandBuffer,
                                              VkPipelineLayout layout,
                                              VkShaderStageFlags stageFlags,
                                              uint32_t offset, uint32_t size,
                                              const void *pValues)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkCmdPushDescriptorSetKHR(
    VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount,
    const VkWriteDescriptorSet *pDescriptorWrites)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkCmdPushDescriptorSetWithTemplateKHR(
    VkCommandBuffer commandBuffer,
    VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    VkPipelineLayout layout, uint32_t set, const void *pData)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorPool(
    VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pDescriptorPool)
{
  (*pDescriptorPool) = new VkDescriptorPool_T;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout(
    VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkDescriptorSetLayout *pSetLayout)
{
  *pSetLayout = new VkDescriptorSetLayout_T;
  for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++)
  {
    const VkDescriptorSetLayoutBinding &Binding = pCreateInfo->pBindings[i];
    (*pSetLayout)->BindingCounts[Binding.binding] = Binding.descriptorCount;
    (*pSetLayout)->BindingTypes[Binding.binding] = Binding.descriptorType;
  }
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorUpdateTemplate(
    VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate)
{
  *pDescriptorUpdateTemplate = new VkDescriptorUpdateTemplate_T;

  // Copy descriptor update entries.
  uint32_t EntryCount = pCreateInfo->descriptorUpdateEntryCount;
  (*pDescriptorUpdateTemplate)->EntryCount = EntryCount;
  (*pDescriptorUpdateTemplate)->Entries =
      new VkDescriptorUpdateTemplateEntry[EntryCount];
  memcpy((*pDescriptorUpdateTemplate)->Entries,
         pCreateInfo->pDescriptorUpdateEntries,
         EntryCount * sizeof(VkDescriptorUpdateTemplateEntry));

  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorUpdateTemplateKHR(
    VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate)
{
  return vkCreateDescriptorUpdateTemplate(device, pCreateInfo, pAllocator,
                                          pDescriptorUpdateTemplate);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineLayout(
    VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout)
{
  // TODO: Implement?
  *pPipelineLayout = new VkPipelineLayout_T;
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                        const VkAllocationCallbacks *pAllocator)
{
  if (descriptorPool)
  {
    for (auto DS : descriptorPool->Pool)
      delete DS;
    delete descriptorPool;
  }
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorSetLayout(
    VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
    const VkAllocationCallbacks *pAllocator)
{
  delete descriptorSetLayout;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorUpdateTemplate(
    VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    const VkAllocationCallbacks *pAllocator)
{
  if (descriptorUpdateTemplate)
  {
    delete[] descriptorUpdateTemplate->Entries;
    delete descriptorUpdateTemplate;
  }
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorUpdateTemplateKHR(
    VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    const VkAllocationCallbacks *pAllocator)
{
  vkDestroyDescriptorUpdateTemplate(device, descriptorUpdateTemplate,
                                    pAllocator);
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout,
                        const VkAllocationCallbacks *pAllocator)
{
  delete pipelineLayout;
}

VKAPI_ATTR VkResult VKAPI_CALL vkFreeDescriptorSets(
    VkDevice device, VkDescriptorPool descriptorPool,
    uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets)
{
  for (uint32_t i = 0; i < descriptorSetCount; i++)
  {
    descriptorPool->Pool.erase(pDescriptorSets[i]);
    delete pDescriptorSets[i];
  }
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetLayoutSupport(
    VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
    VkDescriptorSetLayoutSupport *pSupport)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetLayoutSupportKHR(
    VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
    VkDescriptorSetLayoutSupport *pSupport)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                      VkDescriptorPoolResetFlags flags)
{
  for (auto &DS : descriptorPool->Pool)
    delete DS;

  descriptorPool->Pool.clear();

  return VK_SUCCESS;
}

// Helper to update the descriptors for a particular descriptor set.
// GetDescriptorInfo is a lambda that returns a pointer to the descriptor info
// structure at a particular binding.
template <typename Func>
void updateDescriptors(VkDescriptorSet Set, VkDescriptorType Type,
                       uint32_t Binding, uint32_t ArrayElement, uint32_t Count,
                       const Func &GetDescriptorInfo)
{
  // TODO: Handle other types of descriptors.
  assert(Type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
         Type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
         Type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
         Type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

  for (uint32_t b = 0; b < Count; b++)
  {
    // Check if the current binding is complete.
    assert(Set->Layout->BindingCounts.count(Binding));
    if (ArrayElement >= Set->Layout->BindingCounts.at(Binding))
    {
      ArrayElement = 0;

      // Increment binding, skipping any that have a descriptor count of 0.
      while (Set->Layout->BindingCounts.at(++Binding) == 0)
        ;
    }

    // Get the descriptor info structure.
    const VkDescriptorBufferInfo *BufferInfo =
        (const VkDescriptorBufferInfo *)GetDescriptorInfo(b);

    // Set address for target binding and array element.
    uint64_t Address = BufferInfo->buffer->Address + BufferInfo->offset;
    Set->DescriptorSet[{Binding, ArrayElement}] = Address;

    ++ArrayElement;
  }
}

VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSetWithTemplate(
    VkDevice device, VkDescriptorSet descriptorSet,
    VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData)
{
  for (uint32_t i = 0; i < descriptorUpdateTemplate->EntryCount; i++)
  {
    const VkDescriptorUpdateTemplateEntry &Entry =
        descriptorUpdateTemplate->Entries[i];

    // Return a pointer to the descriptor info for a specific binding.
    auto GetDescriptorInfo = [pData, Entry](uint32_t Binding) -> const void * {
      return ((uint8_t *)pData) + Entry.offset + Entry.stride * Binding;
    };

    updateDescriptors(descriptorSet, Entry.descriptorType, Entry.dstBinding,
                      Entry.dstArrayElement, Entry.descriptorCount,
                      GetDescriptorInfo);
  }
}

VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSetWithTemplateKHR(
    VkDevice device, VkDescriptorSet descriptorSet,
    VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData)
{
  vkUpdateDescriptorSetWithTemplate(device, descriptorSet,
                                    descriptorUpdateTemplate, pData);
}

VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(
    VkDevice device, uint32_t descriptorWriteCount,
    const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount,
    const VkCopyDescriptorSet *pDescriptorCopies)
{
  // TODO: Handle copies.
  assert(descriptorCopyCount == 0 && "descriptor set copies not implemented");

  for (uint32_t i = 0; i < descriptorWriteCount; i++)
  {
    // Return a pointer to the descriptor info for a specific binding.
    VkWriteDescriptorSet Write = pDescriptorWrites[i];
    auto GetDescriptorInfo = [Write](uint32_t Binding) -> const void * {
      return &(Write.pBufferInfo[Binding]);
    };

    updateDescriptors(
        pDescriptorWrites[i].dstSet, pDescriptorWrites[i].descriptorType,
        pDescriptorWrites[i].dstBinding, pDescriptorWrites[i].dstArrayElement,
        pDescriptorWrites[i].descriptorCount, GetDescriptorInfo);
  }
}
