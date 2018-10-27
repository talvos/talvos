// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

#include "talvos/ComputePipeline.h"
#include "talvos/GraphicsPipeline.h"
#include "talvos/Module.h"
#include "talvos/PipelineStage.h"
#include "talvos/Type.h"

// Values match SPIR-V spec.
#define EXEC_MODEL_VERTEX 0
#define EXEC_MODEL_FRAGMENT 4
#define EXEC_MODEL_GLCOMPUTE 5

VKAPI_ATTR void VKAPI_CALL
vkCmdBindPipeline(VkCommandBuffer commandBuffer,
                  VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
  switch (pipelineBindPoint)
  {
  case VK_PIPELINE_BIND_POINT_GRAPHICS:
    commandBuffer->PipelineContext.bindGraphicsPipeline(
        pipeline->GraphicsPipeline);
    break;
  case VK_PIPELINE_BIND_POINT_COMPUTE:
    commandBuffer->PipelineContext.bindComputePipeline(
        pipeline->ComputePipeline);
    break;
  default:
    assert(false && "invalid pipeline bind point");
  }
}

// Helper to generate a specialization constant map for a pipeline stage.
void genSpecConstantMap(const talvos::Module *Mod,
                        const VkSpecializationInfo *SpecInfo,
                        talvos::SpecConstantMap &SM)
{
  if (!SpecInfo)
    return;

  const uint8_t *Data = (const uint8_t *)SpecInfo->pData;
  for (uint32_t s = 0; s < SpecInfo->mapEntryCount; s++)
  {
    const VkSpecializationMapEntry &Entry = SpecInfo->pMapEntries[s];

    uint32_t ResultId = Mod->getSpecConstant(Entry.constantID);
    if (!ResultId)
      continue;

    const talvos::Type *Ty = Mod->getObject(ResultId).getType();
    if (Ty->isBool())
    {
      bool Value = *(VkBool32 *)(Data + Entry.offset) ? true : false;
      SM[Entry.constantID] = talvos::Object(Ty, Value);
    }
    else
    {
      assert(Ty->getSize() == Entry.size);
      SM[Entry.constantID] = talvos::Object(Ty, Data + Entry.offset);
    }
  }
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines(
    VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
    const VkComputePipelineCreateInfo *pCreateInfos,
    const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
  for (uint32_t i = 0; i < createInfoCount; i++)
  {
    const VkPipelineShaderStageCreateInfo &StageInfo = pCreateInfos[i].stage;
    std::shared_ptr<talvos::Module> Mod = StageInfo.module->Module;

    // Build specialization constant map.
    talvos::SpecConstantMap SM;
    genSpecConstantMap(Mod.get(), StageInfo.pSpecializationInfo, SM);

    // Create pipeline.
    pPipelines[i] = new VkPipeline_T;
    talvos::PipelineStage *Stage = new talvos::PipelineStage(
        *device->Device, Mod,
        Mod->getEntryPoint(StageInfo.pName, EXEC_MODEL_GLCOMPUTE), SM);
    pPipelines[i]->ComputePipeline = new talvos::ComputePipeline(Stage);
  }
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(
    VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
    const VkGraphicsPipelineCreateInfo *pCreateInfos,
    const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
  for (uint32_t i = 0; i < createInfoCount; i++)
  {
    talvos::PipelineStage *VertexStage = nullptr;
    talvos::PipelineStage *FragmentStage = nullptr;
    for (uint32_t s = 0; s < pCreateInfos[i].stageCount; s++)
    {
      const VkPipelineShaderStageCreateInfo &StageInfo =
          pCreateInfos[i].pStages[s];
      std::shared_ptr<const talvos::Module> Mod = StageInfo.module->Module;

      // Build specialization constant map.
      talvos::SpecConstantMap SM;
      genSpecConstantMap(Mod.get(), StageInfo.pSpecializationInfo, SM);

      // Create pipeline stage.
      switch (StageInfo.stage)
      {
      case VK_SHADER_STAGE_VERTEX_BIT:
        VertexStage = new talvos::PipelineStage(
            *device->Device, Mod,
            Mod->getEntryPoint(StageInfo.pName, EXEC_MODEL_VERTEX), SM);
        break;
      case VK_SHADER_STAGE_FRAGMENT_BIT:
        FragmentStage = new talvos::PipelineStage(
            *device->Device, Mod,
            Mod->getEntryPoint(StageInfo.pName, EXEC_MODEL_FRAGMENT), SM);
        break;
      default:
        assert(false && "Unhandled pipeline stage");
        break;
      }
    }

    // Set up vertex binding/attribute description lists.
    const VkPipelineVertexInputStateCreateInfo &VertexInfo =
        *pCreateInfos[i].pVertexInputState;
    talvos::VertexBindingDescriptionList VertexBindingDescriptions(
        VertexInfo.pVertexBindingDescriptions,
        VertexInfo.pVertexBindingDescriptions +
            VertexInfo.vertexBindingDescriptionCount);
    talvos::VertexAttributeDescriptionList VertexAttributeDescriptions(
        VertexInfo.pVertexAttributeDescriptions,
        VertexInfo.pVertexAttributeDescriptions +
            VertexInfo.vertexAttributeDescriptionCount);

    // TODO: Handle dynamic state
    assert(!pCreateInfos[i].pDynamicState);

    // Get lists of viewports and scissors.
    const VkPipelineViewportStateCreateInfo &ViewportInfo =
        *pCreateInfos[i].pViewportState;
    assert(ViewportInfo.viewportCount == ViewportInfo.scissorCount);
    std::vector<VkViewport> Viewports(ViewportInfo.pViewports,
                                      ViewportInfo.pViewports +
                                          ViewportInfo.viewportCount);
    std::vector<VkRect2D> Scissors(ViewportInfo.pScissors,
                                   ViewportInfo.pScissors +
                                       ViewportInfo.scissorCount);

    // Create pipeline.
    pPipelines[i] = new VkPipeline_T;
    pPipelines[i]->GraphicsPipeline = new talvos::GraphicsPipeline(
        pCreateInfos[i].pInputAssemblyState->topology, VertexStage,
        FragmentStage, VertexBindingDescriptions, VertexAttributeDescriptions,
        *pCreateInfos[i].pRasterizationState, Viewports, Scissors);
  }
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineCache(
    VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkPipelineCache *pPipelineCache)
{
  *pPipelineCache = new VkPipelineCache_T;
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyPipeline(VkDevice device, VkPipeline pipeline,
                  const VkAllocationCallbacks *pAllocator)
{
  if (pipeline)
  {
    delete pipeline->ComputePipeline;
    delete pipeline->GraphicsPipeline;
    delete pipeline;
  }
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache,
                       const VkAllocationCallbacks *pAllocator)
{
  delete pipelineCache;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache,
                       size_t *pDataSize, void *pData)
{
  if (pData == nullptr)
  {
    *pDataSize = 32;
    return VK_SUCCESS;
  }

  if (*pDataSize < 32)
  {
    *pDataSize = 0;
    return VK_INCOMPLETE;
  }

  // Pipeline cache header.
  ((uint32_t *)pData)[0] = 32;
  ((uint32_t *)pData)[1] = VK_PIPELINE_CACHE_HEADER_VERSION_ONE;
  ((uint32_t *)pData)[2] = 0;
  ((uint32_t *)pData)[3] = 0;

  // Pipeline cache UUID.
  ((uint32_t *)pData)[4] = 0;
  ((uint32_t *)pData)[5] = 0;
  ((uint32_t *)pData)[6] = 0;
  ((uint32_t *)pData)[7] = 0;

  *pDataSize = 32;

  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache,
                      uint32_t srcCacheCount, const VkPipelineCache *pSrcCaches)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}
