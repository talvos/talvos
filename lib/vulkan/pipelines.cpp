// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

#include "talvos/Module.h"
#include "talvos/Pipeline.h"

VKAPI_ATTR void VKAPI_CALL
vkCmdBindPipeline(VkCommandBuffer commandBuffer,
                  VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
  switch (pipelineBindPoint)
  {
  case VK_PIPELINE_BIND_POINT_GRAPHICS:
    commandBuffer->PipelineGraphics = pipeline;
    break;
  case VK_PIPELINE_BIND_POINT_COMPUTE:
    commandBuffer->PipelineCompute = pipeline;
    break;
  default:
    assert(false && "invalid pipeline bind point");
  }
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines(
    VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
    const VkComputePipelineCreateInfo *pCreateInfos,
    const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
  for (uint32_t i = 0; i < createInfoCount; i++)
  {
    const VkPipelineShaderStageCreateInfo &Stage = pCreateInfos[i].stage;
    const talvos::Module *Mod = Stage.module->Module.get();

    // Build specialization constant map if necessary.
    talvos::SpecConstantMap SM;
    if (Stage.pSpecializationInfo)
    {
      const uint8_t *Data = (const uint8_t *)Stage.pSpecializationInfo->pData;
      for (uint32_t s = 0; s < Stage.pSpecializationInfo->mapEntryCount; s++)
      {
        const VkSpecializationMapEntry &Entry =
            Stage.pSpecializationInfo->pMapEntries[s];

        uint32_t ResultId = Mod->getSpecConstant(Entry.constantID);
        if (!ResultId)
          continue;

        const talvos::Type *Ty = Mod->getObject(ResultId).getType();
        assert(Ty->getSize() == Entry.size);

        SM[Entry.constantID] = talvos::Object(Ty, Data + Entry.offset);
      }
    }

    // Create pipeline.
    pPipelines[i] = new VkPipeline_T;
    pPipelines[i]->Pipeline = new talvos::Pipeline(
        *device->Device, Mod, Mod->getEntryPoint(pCreateInfos[i].stage.pName),
        SM);
  }
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(
    VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
    const VkGraphicsPipelineCreateInfo *pCreateInfos,
    const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineCache(
    VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkPipelineCache *pPipelineCache)
{
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyPipeline(VkDevice device, VkPipeline pipeline,
                  const VkAllocationCallbacks *pAllocator)
{
  delete pipeline->Pipeline;
  delete pipeline;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache,
                       const VkAllocationCallbacks *pAllocator)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache,
                       size_t *pDataSize, void *pData)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache,
                      uint32_t srcCacheCount, const VkPipelineCache *pSrcCaches)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}
