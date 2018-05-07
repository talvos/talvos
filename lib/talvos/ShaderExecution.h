// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file ShaderExecution.h
/// This file declares the ShaderExecution class.

#ifndef TALVOS_SHADEREXECUTION_H
#define TALVOS_SHADEREXECUTION_H

#include <atomic>
#include <map>
#include <mutex>
#include <vector>

#include "talvos/Dim3.h"

namespace talvos
{

class Device;
class DispatchCommand;
class Invocation;
class Workgroup;

/// An internal class that handles shader execution, including the interactive
/// debugger.
class ShaderExecution
{
public:
  /// Create a shader execution for \p Command on \p Dev.
  ShaderExecution(Device &Dev, const DispatchCommand &Command);

  // Do not allow ShaderExecution objects to be copied.
  ///\{
  ShaderExecution(const ShaderExecution &) = delete;
  ShaderExecution &operator=(const ShaderExecution &) = delete;
  ///\}

  /// Returns the command that is being executed.
  const DispatchCommand &getCommand() const { return Command; }

  /// Returns the current invocation being executed.
  const Invocation *getCurrentInvocation() const;

  /// Returns the current workgroup being executed.
  const Workgroup *getCurrentWorkgroup() const;

  /// Returns true if the calling thread is a ShaderExecution worker thread.
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

  /// The command being executed.
  const DispatchCommand &Command;

  /// The number of worker threads currently executing.
  unsigned NumThreads;

  /// Index of next group to run in PendingGroups.
  std::atomic<size_t> NextGroupIndex;

  /// Pool of group IDs pending creation and execution.
  std::vector<Dim3> PendingGroups;

  /// Pool of groups that have begun execution and been suspended.
  std::vector<Workgroup *> RunningGroups;

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
