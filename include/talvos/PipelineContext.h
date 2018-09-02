// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file PipelineContext.h
/// This file declares the PipelineContext class and related typedefs.

#ifndef TALVOS_PIPELINECONTEXT_H
#define TALVOS_PIPELINECONTEXT_H

#include <map>
#include <vector>

#include "vulkan/vulkan_core.h"

namespace talvos
{

class ComputePipeline;
class GraphicsPipeline;

/// Mapping from binding indexes to device memory addresses for vertex buffers.
typedef std::map<uint32_t, uint64_t> VertexBindingMap;

/// Mapping from a binding and array element index to an address in memory.
typedef std::map<std::pair<uint32_t, uint32_t>, uint64_t> DescriptorSet;

/// Mapping from set numbers to descriptor sets.
typedef std::map<uint32_t, talvos::DescriptorSet> DescriptorSetMap;

/// This class encapsulates pipeline state and bound resources.
/// A instance of this class used by compute and draw commands when launching
/// shaders.
class PipelineContext
{
public:
  /// Bind descriptor sets used for compute commands.
  void bindComputeDescriptors(const DescriptorSetMap &DSM);

  /// Bind a compute pipeline.
  void bindComputePipeline(const ComputePipeline *PL);

  /// Bind descriptor sets used for draw commands.
  void bindGraphicsDescriptors(const DescriptorSetMap &DSM);

  /// Bind a graphics pipeline.
  void bindGraphicsPipeline(const GraphicsPipeline *PL);

  /// Bind a vertex buffer.
  void bindVertexBuffer(uint32_t Binding, uint64_t Address);

  /// Clear the pipeline context.
  void clear();

  /// Returns the descriptor bindings for compute commands.
  const DescriptorSetMap &getComputeDescriptors() const { return ComputeDSM; }

  /// Returns the compute pipeline.
  const ComputePipeline *getComputePipeline() const { return ComputePL; }

  /// Returns the descriptor bindings for draw commands.
  const DescriptorSetMap &getGraphicsDescriptors() const { return GraphicsDSM; }

  /// Returns the graphics pipeline.
  const GraphicsPipeline *getGraphicsPipeline() const { return GraphicsPL; }

  /// Returns the scissor rectangles.
  const std::vector<VkRect2D> &getScissors() const { return Scissors; }

  /// Returns the vertex bindings.
  const VertexBindingMap &getVertexBindings() const { return VertexBindings; }

private:
  const ComputePipeline *ComputePL = nullptr;   ///< The compute pipeline.
  const GraphicsPipeline *GraphicsPL = nullptr; ///< The graphics pipeline.

  DescriptorSetMap ComputeDSM;  ///< The descriptors for compute commands.
  DescriptorSetMap GraphicsDSM; ///< The descriptors for draw commands.

  std::vector<VkRect2D> Scissors; ///< The scissor rectangles.

  VertexBindingMap VertexBindings; ///< The vertex bindings.
};

}; // namespace talvos

#endif
