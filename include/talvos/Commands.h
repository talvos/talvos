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

#include "talvos/Dim3.h"
#include "talvos/PipelineContext.h"

namespace talvos
{

class Device;
class ComputePipeline;
class GraphicsPipeline;
class Image;
class Object;
class RenderPassInstance;

/// This class is a base class for all commands.
class Command
{
public:
  /// Identifies different Command subclasses.
  enum Type
  {
    BEGIN_RENDER_PASS,
    CLEAR_COLOR_IMAGE,
    COPY_BUFFER,
    COPY_BUFFER_TO_IMAGE,
    COPY_IMAGE,
    COPY_IMAGE_TO_BUFFER,
    DISPATCH,
    DRAW,
    DRAW_INDEXED,
    END_RENDER_PASS,
    FILL_BUFFER,
    NEXT_SUBPASS,
    SET_EVENT,
    RESET_EVENT,
    UPDATE_BUFFER,
    WAIT_EVENTS,
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

/// This class encapsulates information about a clear color image command.
class ClearColorImageCommand : public Command
{
public:
  /// Create a new ClearColorImageCommand.
  ClearColorImageCommand(const Image &DstImage, VkClearColorValue Color,
                         const std::vector<VkImageSubresourceRange> &Ranges)
      : Command(CLEAR_COLOR_IMAGE), DstImage(DstImage), Color(Color),
        Ranges(Ranges)
  {}

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  // The image to clear.
  const Image &DstImage;

  /// The clear color to use.
  VkClearColorValue Color;

  /// The image subranges to clear.
  std::vector<VkImageSubresourceRange> Ranges;
};

/// This class encapsulates information about a copy buffer command.
class CopyBufferCommand : public Command
{
public:
  /// Create a new CopyBufferCommand.
  CopyBufferCommand(uint64_t SrcAddr, uint64_t DstAddr,
                    const std::vector<VkBufferCopy> &Regions)
      : Command(COPY_BUFFER), SrcAddr(SrcAddr), DstAddr(DstAddr),
        Regions(Regions)
  {}

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  // The memory address of the source buffer.
  uint64_t SrcAddr;

  /// The memory address of the destination buffer.
  uint64_t DstAddr;

  /// The regions to copy.
  std::vector<VkBufferCopy> Regions;
};

/// This class encapsulates information about a copy buffer to image command.
class CopyBufferToImageCommand : public Command
{
public:
  /// Create a new CopyBufferToImageCommand.
  CopyBufferToImageCommand(uint64_t SrcAddr, const Image &DstImage,
                           const std::vector<VkBufferImageCopy> &Regions)
      : Command(COPY_BUFFER_TO_IMAGE), SrcAddr(SrcAddr), DstImage(DstImage),
        Regions(Regions)
  {}

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  // The memory address of the source buffer.
  uint64_t SrcAddr;

  /// The destination image.
  const Image &DstImage;

  /// The regions to copy.
  std::vector<VkBufferImageCopy> Regions;
};

/// This class encapsulates information about a copy image command.
class CopyImageCommand : public Command
{
public:
  /// Create a new CopyImageCommand.
  CopyImageCommand(const Image &SrcImage, const Image &DstImage,
                   const std::vector<VkImageCopy> &Regions)
      : Command(COPY_IMAGE), SrcImage(SrcImage), DstImage(DstImage),
        Regions(Regions)
  {}

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  /// The source image.
  const Image &SrcImage;

  /// The destination image.
  const Image &DstImage;

  /// The regions to copy.
  std::vector<VkImageCopy> Regions;
};

/// This class encapsulates information about a copy image to buffer command.
class CopyImageToBufferCommand : public Command
{
public:
  /// Create a new CopyImageToBufferCommand.
  CopyImageToBufferCommand(const Image &SrcImage, uint64_t DstAddr,
                           const std::vector<VkBufferImageCopy> &Regions)
      : Command(COPY_IMAGE_TO_BUFFER), SrcImage(SrcImage), DstAddr(DstAddr),
        Regions(Regions)
  {}

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  // The source image.
  const Image &SrcImage;

  /// The memory address of the destination buffer.
  uint64_t DstAddr;

  /// The regions to copy.
  std::vector<VkBufferImageCopy> Regions;
};

/// This class encapsulates information about a compute kernel launch.
class DispatchCommand : public Command
{
public:
  /// Create a new DispatchCommand.
  ///
  /// \param PC The pipeline context used for the shader launch.
  /// \param NumGroups The number of groups to launch.
  DispatchCommand(const PipelineContext &PC, Dim3 NumGroups)
      : Command(DISPATCH), PC(PC), NumGroups(NumGroups)
  {}

  /// Returns the number of workgroups this command launches.
  Dim3 getNumGroups() const { return NumGroups; }

  /// Returns the pipeline context.
  const PipelineContext &getPipelineContext() const { return PC; }

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  PipelineContext PC; ///< The pipeline context.

  Dim3 NumGroups; ///< The number of workgroups to launch.
};

/// This is an abstract base class for draw commands.
class DrawCommandBase : public Command
{
public:
  /// Create a new DrawCommandBase.
  ///
  /// \param PC The pipeline context used for the draw command.
  /// \param RPI The render pass instance to use.
  /// \param NumVertices The number of vertices to draw.
  /// \param VertexOffset The offset of the first vertex.
  /// \param NumInstances The number of instances to draw.
  /// \param InstanceOffset The offset of the first instance.
  DrawCommandBase(Type Ty, const PipelineContext &PC,
                  const std::shared_ptr<RenderPassInstance> RPI,
                  uint32_t NumVertices, uint32_t VertexOffset,
                  uint32_t NumInstances, uint32_t InstanceOffset)
      : Command(Ty), PC(PC), RPI(RPI), NumVertices(NumVertices),
        VertexOffset(VertexOffset), NumInstances(NumInstances),
        InstanceOffset(InstanceOffset)
  {}

  /// Returns the offset of the first instance.
  uint32_t getInstanceOffset() const { return InstanceOffset; }

  /// Returns the number of instances.
  uint32_t getNumInstances() const { return NumInstances; }

  /// Returns the number of vertices.
  uint32_t getNumVertices() const { return NumVertices; }

  /// Returns the pipeline context.
  const PipelineContext &getPipelineContext() const { return PC; }

  /// Returns the render pass instance used by this command.
  const RenderPassInstance &getRenderPassInstance() const { return *RPI; }

  /// Returns the offset of the first vertex.
  uint32_t getVertexOffset() const { return VertexOffset; }

  /// Set the render pass instance used by this command.
  void setRenderPassInstance(const std::shared_ptr<RenderPassInstance> RPI)
  {
    this->RPI = RPI;
  }

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override = 0;

private:
  PipelineContext PC; ///< The pipeline context.

  std::shared_ptr<RenderPassInstance> RPI; ///< The render pass instance.

  uint32_t NumVertices;    ///< Number of vertices.
  uint32_t VertexOffset;   ///< Offset of first vertex.
  uint32_t NumInstances;   ///< Number of instances.
  uint32_t InstanceOffset; ///< Offset of first instance.
};

/// This class encapsulates information about a draw command.
class DrawCommand : public DrawCommandBase
{
public:
  /// Create a new DrawCommand.
  ///
  /// \param PC The pipeline context used for the draw command.
  /// \param RPI The render pass instance to use.
  /// \param NumVertices The number of vertices to draw.
  /// \param VertexOffset The offset of the first vertex.
  /// \param NumInstances The number of instances to draw.
  /// \param InstanceOffset The offset of the first instance.
  DrawCommand(const PipelineContext &PC,
              const std::shared_ptr<RenderPassInstance> RPI,
              uint32_t NumVertices, uint32_t VertexOffset,
              uint32_t NumInstances, uint32_t InstanceOffset)
      : DrawCommandBase(DRAW, PC, RPI, NumVertices, VertexOffset, NumInstances,
                        InstanceOffset){};

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;
};

/// This class encapsulates information about an indexed draw command.
class DrawIndexedCommand : public DrawCommandBase
{
public:
  /// Create a new DrawIndexedCommand.
  ///
  /// \param PC The pipeline context used for the draw command.
  /// \param RPI The render pass instance to use.
  /// \param NumVertices The number of vertices to draw.
  /// \param IndexOffset The offset of the first index.
  /// \param VertexOffset The offset of the first vertex.
  /// \param NumInstances The number of instances to draw.
  /// \param InstanceOffset The offset of the first instance.
  /// \param IndexBaseAddress The address in memory of the indices.
  /// \param IndexType The type of the indices.
  DrawIndexedCommand(const PipelineContext &PC,
                     const std::shared_ptr<RenderPassInstance> RPI,
                     uint32_t NumVertices, uint32_t IndexOffset,
                     uint32_t VertexOffset, uint32_t NumInstances,
                     uint32_t InstanceOffset, uint64_t IndexBaseAddress,
                     VkIndexType IndexType)
      : DrawCommandBase(DRAW_INDEXED, PC, RPI, NumVertices, VertexOffset,
                        NumInstances, InstanceOffset),
        IndexOffset(IndexOffset), IndexBaseAddress(IndexBaseAddress),
        IndexType(IndexType){};

  /// Returns the address in memory of the indices.
  uint64_t getIndexBaseAddress() const { return IndexBaseAddress; }

  /// Returns the offset of the first index.
  uint32_t getIndexOffset() const { return IndexOffset; }

  /// Returns the type of the indices.
  VkIndexType getIndexType() const { return IndexType; }

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  uint32_t IndexOffset;      ///< Offset of the first index.
  uint64_t IndexBaseAddress; ///< Address of the indices.
  VkIndexType IndexType;     ///< Type of the indices;
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

/// This class encapsulates information about a fill buffer command.
class FillBufferCommand : public Command
{
public:
  /// Create a new FillBufferCommand.
  FillBufferCommand(uint64_t Base, uint64_t NumBytes, uint32_t Data)
      : Command(FILL_BUFFER), Base(Base), NumBytes(NumBytes), Data(Data)
  {}

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  // The memory address to begin filling from.
  uint64_t Base;

  /// The number of bytes to fill.
  uint64_t NumBytes;

  /// The data to fill the buffer with.
  uint32_t Data;
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

/// This class encapsulates information about a reset event command.
class ResetEventCommand : public Command
{
public:
  /// Create a new ResetEventCommand.
  ResetEventCommand(volatile bool *Event) : Command(RESET_EVENT), Event(Event)
  {}

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  /// The flag to reset when this command executes.
  volatile bool *Event;
};

/// This class encapsulates information about a set event command.
class SetEventCommand : public Command
{
public:
  /// Create a new SetEventCommand.
  SetEventCommand(volatile bool *Event) : Command(SET_EVENT), Event(Event) {}

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  /// The flag to set when this command executes.
  volatile bool *Event;
};

/// This class encapsulates information about an update buffer command.
class UpdateBufferCommand : public Command
{
public:
  /// Create a new UpdateBufferCommand.
  UpdateBufferCommand(uint64_t Base, uint64_t NumBytes, const void *Data);

  ~UpdateBufferCommand();

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  // The memory address to begin updating from.
  uint64_t Base;

  /// The number of bytes to update.
  uint64_t NumBytes;

  /// The data to update the buffer with.
  uint8_t *Data;
};

/// This class encapsulates information about a wait events command.
class WaitEventsCommand : public Command
{
public:
  /// Create a new WaitEventsCommand.
  WaitEventsCommand(std::vector<volatile bool *> Events)
      : Command(WAIT_EVENTS), Events(Events)
  {}

protected:
  /// Command execution handler.
  virtual void runImpl(Device &Dev) const override;

private:
  /// The events to wait for.
  std::vector<volatile bool *> Events;
};

} // namespace talvos

#endif
