// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Commands.h
/// This file declares the Command base class and its subclasses.

#ifndef TALVOS_COMMANDS_H
#define TALVOS_COMMANDS_H

#include "talvos/DescriptorSet.h"
#include "talvos/Dim3.h"

namespace talvos
{

class ComputePipeline;
class GraphicsPipeline;
class Object;

/// Mapping from binding indexes to device memory addresses for vertex buffers.
typedef std::map<uint32_t, uint64_t> VertexBindingMap;

/// This class is a base class for all commands.
class Command
{
public:
  /// Identifies different Command subclasses.
  enum Type
  {
    DISPATCH,
    DRAW,
  };

  /// Returns the type of this command.
  Type getType() const { return Ty; }

protected:
  /// Used by subclasses to initialize the command type.
  Command(Type Ty) : Ty(Ty){};

  Type Ty; ///< The type of this command.
};

/// This class encapsulates information about a compute kernel launch.
class DispatchCommand : public Command
{
public:
  /// Create a new DispatchCommand.
  ///
  /// Any buffers used by \p PL must have a corresponding entry in \p DSM.
  ///
  /// \param PL The compute pipeline to invoke.
  /// \param NumGroups The number of groups to launch.
  /// \param DSM The descriptor set mapping to use.
  DispatchCommand(const ComputePipeline *PL, Dim3 NumGroups,
                  const DescriptorSetMap &DSM)
      : Command(DISPATCH), Pipeline(PL), NumGroups(NumGroups), DSM(DSM)
  {}

  /// Returns the descriptor set map used by the command.
  const DescriptorSetMap &getDescriptorSetMap() const { return DSM; }

  /// Returns the number of workgroups this command launches.
  Dim3 getNumGroups() const { return NumGroups; }

  /// Returns the pipeline this command is invoking.
  const ComputePipeline *getPipeline() const { return Pipeline; }

private:
  const ComputePipeline *Pipeline; ///< The pipeline to use.

  Dim3 NumGroups; ///< The number of workgroups to launch.

  DescriptorSetMap DSM; ///< The descriptor set map to use.
};

/// This class encapsulates information about a draw command.
class DrawCommand : public Command
{
public:
  /// Create a new DrawCommand.
  ///
  /// Any buffers used by \p PL must have a corresponding entry in \p DSM.
  ///
  /// \param PL The graphics pipeline to invoke.
  /// \param NumVertices The number of vertices to draw.
  /// \param VertexOffset The offset of the first vertex.
  /// \param NumInstances The number of instances to draw.
  /// \param InstanceOffset The offset of the first instance.
  /// \param DSM The descriptor set mapping to use.
  DrawCommand(const GraphicsPipeline *PL, uint32_t NumVertices,
              uint32_t VertexOffset, uint32_t NumInstances,
              uint32_t InstanceOffset, const DescriptorSetMap &DSM,
              const VertexBindingMap &VertexBindings)
      : Command(DRAW), Pipeline(PL), NumVertices(NumVertices),
        VertexOffset(VertexOffset), NumInstances(NumInstances),
        InstanceOffset(InstanceOffset), DSM(DSM),
        VertexBindings(VertexBindings){};

  /// Returns the descriptor set map used by the command.
  const DescriptorSetMap &getDescriptorSetMap() const { return DSM; }

  /// Returns the offset of the first instance.
  uint32_t getInstanceOffset() const { return InstanceOffset; }

  /// Returns the number of instances.
  uint32_t getNumInstances() const { return NumInstances; }

  /// Returns the number of vertices.
  uint32_t getNumVertices() const { return NumVertices; }

  /// Returns the pipeline this command is invoking.
  const GraphicsPipeline *getPipeline() const { return Pipeline; }

  /// Returns the vertex binding map used by the comand.
  const VertexBindingMap &getVertexBindings() const { return VertexBindings; }

  /// Returns the offset of the first vertex.
  uint32_t getVertexOffset() const { return VertexOffset; }

private:
  const GraphicsPipeline *Pipeline; ///< The pipeline to use.

  uint32_t NumVertices;    ///< Number of vertices.
  uint32_t VertexOffset;   ///< Offset of first vertex.
  uint32_t NumInstances;   ///< Number of instances.
  uint32_t InstanceOffset; ///< Offset of first instance.

  DescriptorSetMap DSM; ///< The descriptor set map to use.

  VertexBindingMap VertexBindings; ///< The vertex buffer bindings to use.
};

} // namespace talvos

#endif
