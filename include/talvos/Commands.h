// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Commands.h
/// This file declares the Command base class and its subclasses.

#ifndef TALVOS_COMMANDS_H
#define TALVOS_COMMANDS_H

#include <memory>
#include <vector>

#include "vulkan/vulkan_core.h"

#include "talvos/DescriptorSet.h"
#include "talvos/Dim3.h"

namespace talvos
{

class Device;
class ComputePipeline;
class GraphicsPipeline;
class Object;
class RenderPassInstance;

/// Mapping from binding indexes to device memory addresses for vertex buffers.
typedef std::map<uint32_t, uint64_t> VertexBindingMap;

/// This class is a base class for all commands.
class Command
{
public:
  /// Identifies different Command subclasses.
  enum Type
  {
    BEGIN_RENDER_PASS,
    COPY_BUFFER_TO_IMAGE,
    COPY_IMAGE_TO_BUFFER,
    DISPATCH,
    DRAW,
    END_RENDER_PASS,
    NEXT_SUBPASS,
  };

  /// Returns the type of this command.
  Type getType() const { return Ty; }

  /// Run this command on \p Dev.
  void run(Device &Dev) const;

protected:
  /// Used by subclasses to initialize the command type.
  Command(Type Ty) : Ty(Ty){};

  Type Ty; ///< The type of this command.

  /// Command execution method for subclasses.
  virtual void runImpl(Device &Dev) const = 0;
};

/// This class encapsulates information about a begin render pass command.
class BeginRenderPassCommand : public Command
{
public:
  /// Create a new BeginRenderPassCommand for a RenderPassInstance.
  BeginRenderPassCommand(std::shared_ptr<RenderPassInstance> RPI)
      : Command(BEGIN_RENDER_PASS), RPI(RPI)
  {}

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  /// The render pass instance.
  std::shared_ptr<RenderPassInstance> RPI;
};

/// This class encapsulates information about a copy buffer to image command.
class CopyBufferToImageCommand : public Command
{
public:
  /// Create a new CopyBufferToImageCommand.
  CopyBufferToImageCommand(uint64_t SrcAddr, uint64_t DstAddr,
                           VkFormat DstFormat, VkExtent3D DstSize,
                           const std::vector<VkBufferImageCopy> &Regions)
      : Command(COPY_BUFFER_TO_IMAGE), SrcAddr(SrcAddr), DstAddr(DstAddr),
        DstFormat(DstFormat), DstSize(DstSize), Regions(Regions)
  {}

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  // The memory address of the source buffer.
  uint64_t SrcAddr;

  /// The memory address of the destination image.
  uint64_t DstAddr;

  /// The format of the destination image.
  VkFormat DstFormat;

  /// The dimensions of the destination image.
  VkExtent3D DstSize;

  /// The regions to copy.
  std::vector<VkBufferImageCopy> Regions;
};

/// This class encapsulates information about a copy image to buffer command.
class CopyImageToBufferCommand : public Command
{
public:
  /// Create a new CopyImageToBufferCommand.
  CopyImageToBufferCommand(uint64_t SrcAddr, uint64_t DstAddr,
                           VkFormat SrcFormat, VkExtent3D SrcSize,
                           const std::vector<VkBufferImageCopy> &Regions)
      : Command(COPY_IMAGE_TO_BUFFER), SrcAddr(SrcAddr), DstAddr(DstAddr),
        SrcFormat(SrcFormat), SrcSize(SrcSize), Regions(Regions)
  {}

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  // The memory address of the source image.
  uint64_t SrcAddr;

  /// The memory address of the destination buffer.
  uint64_t DstAddr;

  /// The format of the source image.
  VkFormat SrcFormat;

  /// The dimensions of the source image.
  VkExtent3D SrcSize;

  /// The regions to copy.
  std::vector<VkBufferImageCopy> Regions;
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

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

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
  /// \param VertexBindings The vertex buffer bindings to use.
  /// \param RPI The render pass instance to use;
  DrawCommand(const GraphicsPipeline *PL, uint32_t NumVertices,
              uint32_t VertexOffset, uint32_t NumInstances,
              uint32_t InstanceOffset, const DescriptorSetMap &DSM,
              const VertexBindingMap &VertexBindings,
              const std::shared_ptr<RenderPassInstance> RPI)
      : Command(DRAW), Pipeline(PL), NumVertices(NumVertices),
        VertexOffset(VertexOffset), NumInstances(NumInstances),
        InstanceOffset(InstanceOffset), DSM(DSM),
        VertexBindings(VertexBindings), RPI(RPI){};

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

  /// Returns the render pass instance used by this command.
  const RenderPassInstance &getRenderPassInstance() const { return *RPI; }

  /// Returns the vertex binding map used by the command.
  const VertexBindingMap &getVertexBindings() const { return VertexBindings; }

  /// Returns the offset of the first vertex.
  uint32_t getVertexOffset() const { return VertexOffset; }

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  const GraphicsPipeline *Pipeline; ///< The pipeline to use.

  uint32_t NumVertices;    ///< Number of vertices.
  uint32_t VertexOffset;   ///< Offset of first vertex.
  uint32_t NumInstances;   ///< Number of instances.
  uint32_t InstanceOffset; ///< Offset of first instance.

  DescriptorSetMap DSM; ///< The descriptor set map to use.

  VertexBindingMap VertexBindings; ///< The vertex buffer bindings to use.

  const std::shared_ptr<RenderPassInstance> RPI; ///< The render pass instance.
};

/// This class encapsulates information about an end render pass command.
class EndRenderPassCommand : public Command
{
public:
  /// Create a new EndRenderPassCommand for a RenderPassInstance.
  EndRenderPassCommand(std::shared_ptr<RenderPassInstance> RPI)
      : Command(END_RENDER_PASS), RPI(RPI)
  {}

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  /// The render pass instance.
  std::shared_ptr<RenderPassInstance> RPI;
};

/// This class encapsulates information about a next subpass command.
class NextSubpassCommand : public Command
{
public:
  /// Create a new NextSubpassCommand for a RenderPassInstance.
  NextSubpassCommand(std::shared_ptr<RenderPassInstance> RPI)
      : Command(NEXT_SUBPASS), RPI(RPI)
  {}

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  /// The render pass instance.
  std::shared_ptr<RenderPassInstance> RPI;
};

} // namespace talvos

#endif
