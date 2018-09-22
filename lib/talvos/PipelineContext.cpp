// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file PipelineContext.cpp
/// This file defines the PipelineContext class.

#include <cassert>
#include <cstring>

#include "talvos/PipelineContext.h"
#include "talvos/GraphicsPipeline.h"

namespace talvos
{

void PipelineContext::bindComputeDescriptors(const DescriptorSetMap &DSM)
{
  for (auto &DS : DSM)
    ComputeDSM[DS.first] = DS.second;
}

void PipelineContext::bindComputePipeline(const ComputePipeline *PL)
{
  ComputePL = PL;
}

void PipelineContext::bindGraphicsDescriptors(const DescriptorSetMap &DSM)
{
  for (auto &DS : DSM)
    GraphicsDSM[DS.first] = DS.second;
}

void PipelineContext::bindGraphicsPipeline(const GraphicsPipeline *PL)
{
  GraphicsPL = PL;
  Viewports = PL->getViewports();
  Scissors = PL->getScissors();
}

void PipelineContext::bindVertexBuffer(uint32_t Binding, uint64_t Address)
{
  VertexBindings[Binding] = Address;
}

void PipelineContext::clear()
{
  ComputePL = nullptr;
  GraphicsPL = nullptr;
  ComputeDSM.clear();
  GraphicsDSM.clear();
  Scissors.clear();
  VertexBindings.clear();
}

void PipelineContext::setPushConstantData(uint32_t Offset, uint32_t NumBytes,
                                          const uint8_t *Data)
{
  assert(Offset + NumBytes <= PUSH_CONSTANT_MEM_SIZE);
  memcpy(PushConstantData + Offset, Data, NumBytes);
}

} // namespace talvos
