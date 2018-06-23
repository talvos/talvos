// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file PipelineExecutor.h
/// This file declares the PipelineExecutor class.

#ifndef TALVOS_PIPELINEEXECUTOR_H
#define TALVOS_PIPELINEEXECUTOR_H

#include <atomic>
#include <map>
#include <mutex>
#include <vector>

#include "talvos/DescriptorSet.h"
#include "talvos/Dim3.h"

namespace talvos
{

class Command;
class Device;
class DispatchCommand;
class DrawCommand;
class Framebuffer;
class Invocation;
class Object;
class PipelineStage;
class RenderPassInstance;
class Variable;
class Workgroup;

/// Only allow Device objects to create PipelineExecutor instances.
class PipelineExecutorKey
{
  friend class Device;
  PipelineExecutorKey(){};
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
  void run(const DrawCommand &Cmd);

  /// Signal that an error has occurred, breaking the interactive debugger.
  void signalError();

private:
  /// Internal structure to hold the state of a render pipeline.
  struct RenderPipelineState;

  /// Internal structure to hold vertex shader output variables.
  struct VertexOutput;

  /// Worker thread entry point for compute shaders.
  void runComputeWorker();

  /// Worker thread entry point for vertex shaders.
  void runVertexWorker(RenderPipelineState *State);

  /// Finalise buffer variables.
  void finaliseBufferVariables(const DescriptorSetMap &DSM);

  /// Initialise buffer variables.
  void initialiseBufferVariables(const DescriptorSetMap &DSM);

  /// Helper function to rasterize a triangle primitive.
  void rasterizeTriangle(const DrawCommand &Cmd, const VertexOutput &VA,
                         const VertexOutput &VB, const VertexOutput &VC);

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
