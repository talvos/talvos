// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "runtime.h"

#include "talvos/ComputePipeline.h"
#include "talvos/GraphicsPipeline.h"
#include "talvos/Module.h"
#include "talvos/PipelineStage.h"

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
    commandBuffer->PipelineGraphics = pipeline;
    commandBuffer->Scissors = pipeline->GraphicsPipeline->getScissors();
    break;
  case VK_PIPELINE_BIND_POINT_COMPUTE:
    commandBuffer->PipelineCompute = pipeline;
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

    // Get list of static scissor rectangles.
    // TODO: Implement multiple viewports
    const VkPipelineViewportStateCreateInfo &ViewportInfo =
        *pCreateInfos[i].pViewportState;
    assert(ViewportInfo.viewportCount == 1);
    std::vector<VkRect2D> Scissors(ViewportInfo.pScissors,
                                   ViewportInfo.pScissors +
                                       ViewportInfo.scissorCount);

    // Create pipeline.
    pPipelines[i] = new VkPipeline_T;
    pPipelines[i]->GraphicsPipeline = new talvos::GraphicsPipeline(
        pCreateInfos[i].pInputAssemblyState->topology, VertexStage,
        FragmentStage, VertexBindingDescriptions, VertexAttributeDescriptions,
        Scissors);
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
  TALVOS_ABORT_UNIMPLEMENTED;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache,
                      uint32_t srcCacheCount, const VkPipelineCache *pSrcCaches)
{
  TALVOS_ABORT_UNIMPLEMENTED;
}
