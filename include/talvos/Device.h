// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Device.h
/// This file declares the Device class.

#ifndef TALVOS_DEVICE_H
#define TALVOS_DEVICE_H

#include <memory>
#include <vector>

#include "talvos/Dim3.h"

namespace talvos
{

class DispatchCommand;
class Invocation;
class Memory;
class Workgroup;

/// A Device instance encapsulates properties and state for the virtual device.
/// It is also responsible for running commands, including the interactive
/// debugger.
class Device
{
public:
  Device();
  ~Device();

  // Do not allow Device objects to be copied.
  ///\{
  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;
  ///\}

  /// Get the global memory instance associated with this device.
  Memory &getGlobalMemory() { return *GlobalMemory; }

  /// Report an error that has occurred during emulation.
  /// This prints \p Error to stderr along with the current execution context.
  void reportError(const std::string &Error);

  /// Run \p Command to completion.
  void run(const DispatchCommand &Command);

private:
  Memory *GlobalMemory; ///< The global memory of this device.

  /// The command currently being executed.
  const DispatchCommand *CurrentCommand;

  /// Index of next group to run in PendingGroups.
  size_t NextGroupIndex;

  /// Pool of group IDs pending creation and execution.
  std::vector<Dim3> PendingGroups;

  /// Pool of groups that have begun execution and been suspended.
  std::vector<Workgroup *> RunningGroups;

  Invocation *CurrentInvocation; ///< The current invocation being executed.
  Workgroup *CurrentGroup;       ///< The current workgroup being executed.

  // Interactive debugging functionality.
  bool Continue;    ///< True when the user has used \p continue command.
  bool Interactive; ///< True when interactive mode is enabled.

  /// Trigger interaction with the user (if necessary).
  void interact();

  /// Print the next instruction that will be executed.
  void printNextInstruction();

  /// \name Interactive command handlers.
  /// Return true when the interpreter should resume executing instructions.
  ///@{
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
