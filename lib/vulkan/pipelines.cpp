// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

#include "talvos/Module.h"

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
    pPipelines[i] = new VkPipeline_T;
    pPipelines[i]->Module = pCreateInfos[i].stage.module->Module.get();
    pPipelines[i]->Function =
        pPipelines[i]->Module->getEntryPoint(pCreateInfos[i].stage.pName);
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
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyPipeline(VkDevice device, VkPipeline pipeline,
                  const VkAllocationCallbacks *pAllocator)

{
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
