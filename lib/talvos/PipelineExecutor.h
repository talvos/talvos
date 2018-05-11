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

class Device;
class Invocation;
class Object;
class PipelineStage;
class Workgroup;

/// An internal class that handles pipeline execution, including the interactive
/// debugger.
class PipelineExecutor
{
public:
  /// Create a shader execution for \p Command on \p Dev.
  PipelineExecutor(Device &Dev, const PipelineStage &Stage,
                   const DescriptorSetMap &DSM, Dim3 NumGroups);

  // Do not allow PipelineExecutor objects to be copied.
  ///\{
  PipelineExecutor(const PipelineExecutor &) = delete;
  PipelineExecutor &operator=(const PipelineExecutor &) = delete;
  ///\}

  /// Returns the current invocation being executed.
  const Invocation *getCurrentInvocation() const;

  /// Returns the current workgroup being executed.
  const Workgroup *getCurrentWorkgroup() const;

  /// Returns the initial object values for each shader invocation.
  const std::vector<Object> &getInitialObjects() const { return Objects; }

  /// Returns the number of groups in this shader execution.
  /// This is only valid for compute shaders.
  Dim3 getNumGroups() const { return NumGroups; }

  /// Returns the pipeline stage that is being executed.
  const PipelineStage &getPipelineStage() const { return Stage; }

  /// Returns true if the calling thread is a PipelineExecutor worker thread.
  bool isWorkerThread() const;

  /// Run the shader to completion.
  void run();

  /// Signal that an error has occurred, breaking the interactive debugger.
  void signalError();

private:
  /// Worker thread entry point.
  void runWorker();

  /// The device this shader is executing on.
  Device &Dev;

  /// The pipeline stage being executed.
  const PipelineStage &Stage;

  /// The initial object values for each invocation.
  std::vector<Object> Objects;

  /// The number of groups in this shader execution.
  Dim3 NumGroups;

  /// The number of worker threads currently executing.
  unsigned NumThreads;

  /// Index of next group to run in PendingGroups.
  std::atomic<size_t> NextGroupIndex;

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
