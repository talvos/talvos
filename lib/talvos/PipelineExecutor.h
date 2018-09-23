// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file PipelineExecutor.h
/// This file declares the PipelineExecutor class.

#ifndef TALVOS_PIPELINEEXECUTOR_H
#define TALVOS_PIPELINEEXECUTOR_H

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include "talvos/Dim3.h"
#include "talvos/PipelineContext.h"

namespace talvos
{

class Command;
class Device;
class DispatchCommand;
class DrawCommandBase;
class Framebuffer;
class Invocation;
class Memory;
class Object;
class PipelineStage;
class RenderPassInstance;
class Type;
class Variable;
class Workgroup;

/// Only allow Device objects to create PipelineExecutor instances.
class PipelineExecutorKey
{
  friend class Device;
  PipelineExecutorKey(){};
};

// TODO: Define proper Vec2/Vec3/Vec4/Vec<N> classes?
struct Vec4
{
  float X, Y, Z, W;
};

/// An internal class that handles pipeline execution, including the interactive
/// debugger.
class PipelineExecutor
{
public:
  /// Create a pipeline executor on \p Dev.
  PipelineExecutor(PipelineExecutorKey Key, Device &Dev);

  // Do not allow PipelineExecutor objects to be copied.
  ///\{
  PipelineExecutor(const PipelineExecutor &) = delete;
  PipelineExecutor &operator=(const PipelineExecutor &) = delete;
  ///\}

  /// Returns the current invocation being executed.
  const Invocation *getCurrentInvocation() const;

  /// Returns the current workgroup being executed.
  const Workgroup *getCurrentWorkgroup() const;

  /// Returns the pipeline stage that is currently being executed.
  const PipelineStage &getCurrentStage() const { return *CurrentStage; }

  /// Returns true if the calling thread is a PipelineExecutor worker thread.
  bool isWorkerThread() const;

  /// Run a compute dispatch command to completion.
  void run(const DispatchCommand &Cmd);

  /// Run a draw command to completion.
  void run(const DrawCommandBase &Cmd);

  /// Signal that an error has occurred, breaking the interactive debugger.
  void signalError();

private:
  /// Internal structure to hold the state of a render pipeline.
  struct RenderPipelineState;

  /// Internal structure to hold vertex shader output variables.
  struct VertexOutput;

  /// Internal structure to hold point primitive data during rasterization.
  struct PointPrimitive;

  /// Internal structure to hold triangle primitive data during rasterization.
  struct TrianglePrimitive;

  /// Internal structure to hold fragment data.
  struct Fragment
  {
    uint32_t X;  ///< Framebuffer x-coordinate.
    uint32_t Y;  ///< Framebuffer y-coordinate.
    float Depth; ///< Fragment depth.
    float InvW;  ///< Inverse of the interpolated clip w coordinate.
  };

  /// Helper function to launch worker threads (created by \p ThreadCreator).
  void runWorkers(std::function<std::thread()> ThreadCreator);

  /// Worker thread entry point for compute shaders.
  void runComputeWorker();

  /// Worker thread entry point for triangle rasterization.
  void runTriangleFragmentWorker(TrianglePrimitive Primitive,
                                 const RenderPassInstance &RPI,
                                 const VkViewport &Viewport);

  /// Worker thread entry point for point rasterization.
  void runPointFragmentWorker(PointPrimitive Primitive,
                              const RenderPassInstance &RPI);

  /// Worker thread entry point for vertex shaders.
  void runVertexWorker(RenderPipelineState *State, uint32_t InstanceIndex);

  /// Finalize variables.
  void finalizeVariables(const DescriptorSetMap &DSM);

  /// Initialize variables.
  void initializeVariables(const DescriptorSetMap &DSM,
                           uint64_t PushConstantAddress);

  /// Helper function to build list of pending fragments in a bounding box.
  void buildPendingFragments(const DrawCommandBase &Cmd, int XMinFB, int XMaxFB,
                             int YMinFB, int YMaxFB);

  /// Helper function to process a fragment.
  void processFragment(const Fragment &Frag, const RenderPassInstance &RPI,
                       std::function<void(uint32_t, uint32_t, const Variable *,
                                          const Type *, Memory *, uint64_t)>
                           GenLocData);

  /// Helper function to rasterize a point primitive.
  void rasterizePoint(const DrawCommandBase &Cmd, const VkViewport &Viewport,
                      const VertexOutput &Vertex);

  /// Helper function to rasterize a triangle primitive.
  void rasterizeTriangle(const DrawCommandBase &Cmd, const VkViewport &Viewport,
                         const VertexOutput &VA, const VertexOutput &VB,
                         const VertexOutput &VC);

  /// Helper function to get the position from vertex output builtin data.
  static Vec4 getPosition(const VertexOutput &Out);

  /// The device this shader is executing on.
  Device &Dev;

  /// The command currently being executed.
  const Command *CurrentCommand;

  /// The pipeline stage currently being executed.
  const PipelineStage *CurrentStage;

  /// The initial object values for each invocation.
  std::vector<Object> Objects;

  /// The number of worker threads currently executing.
  unsigned NumThreads;

  /// Index of next item of work to execute.
  std::atomic<size_t> NextWorkIndex;

  /// Pool of group IDs pending creation and execution.
  std::vector<Dim3> PendingGroups;

  /// Pool of groups that have begun execution and been suspended.
  std::vector<Workgroup *> RunningGroups;

  /// Pool of framebuffer coordinates pending fragment processing.
  std::vector<Dim3> PendingFragments;

  /// Create a compute shader workgroup and its work-item invocations.
  Workgroup *createWorkgroup(Dim3 GroupId) const;

  // Interactive debugging functionality.
  bool Continue;    ///< True when the user has used \p continue command.
  bool Interactive; ///< True when interactive mode is enabled.

  /// Trigger interaction with the user (if necessary).
  void interact();

  /// Print the context for the current invocation.
  void printContext() const;

  /// Tokens for the most recent interactive command entered.
  std::vector<std::string> LastLine;

  /// Index of the next breakpoint to create.
  static uint32_t NextBreakpoint;

  /// Map from breakpoint ID to instruction result ID.
  static std::map<uint32_t, uint32_t> Breakpoints;

  /// \name Interactive command handlers.
  /// Return true when the interpreter should resume executing instructions.
  ///@{
  bool brk(const std::vector<std::string> &Args);
  bool breakpoint(const std::vector<std::string> &Args);
  bool cont(const std::vector<std::string> &Args);
  bool help(const std::vector<std::string> &Args);
  bool print(const std::vector<std::string> &Args);
  bool quit(const std::vector<std::string> &Args);
  bool step(const std::vector<std::string> &Args);
  bool swtch(const std::vector<std::string> &Args);
  ///@}
};

} // namespace talvos

#endif
