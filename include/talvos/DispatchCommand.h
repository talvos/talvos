// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_DISPATCHCOMMAND_H
#define TALVOS_DISPATCHCOMMAND_H

#include <map>
#include <vector>

#include "talvos/Dim3.h"

namespace talvos
{

class Device;
class Function;
class Invocation;
class Module;
class Object;
class Workgroup;

/// Map from Descriptor/Binding pair to an address in memory.
typedef std::map<std::pair<uint32_t, uint32_t>, uint64_t> DescriptorSet;

/// This class is used to invoke compute kernels on a virtual device.
class DispatchCommand
{
public:
  /// List of mappings from SPIR-V result ID to Object for each variable.
  typedef std::vector<std::pair<uint32_t, Object>> VariableList;

  /// Create a new DispatchCommand.
  ///
  /// The function \p F must belong to module \p M. Any buffers used by \p F
  /// must have a corresponding entry in \p DS.
  ///
  /// \param D The target device.
  /// \param M The module containing the entry point to invoke.
  /// \param F The entry point to invoke.
  /// \param NumGroups The number of groups to launch.
  /// \param DS The descriptor set mapping to use.
  DispatchCommand(Device *D, const Module *M, const Function *F, Dim3 NumGroups,
                  const DescriptorSet &DS);

  // Do not allow DispatchCommand objects to be copied.
  ///\{
  DispatchCommand(const DispatchCommand &) = delete;
  DispatchCommand &operator=(const DispatchCommand &) = delete;
  ///\}

  /// Return the device this command is targeting.
  Device *getDevice() const { return Dev; }

  /// Return the function this command is invoking.
  const Function *getFunction() const { return Func; }

  /// Return the workgroup size.
  Dim3 getGroupSize() const { return GroupSize; }

  /// Return the number of workgroups.
  Dim3 getNumGroups() const { return NumGroups; }

  /// Return the module this command is using.
  const Module *getModule() const { return Mod; }

  /// Return the resolved buffer variable pointer values for this command.
  const VariableList &getVariables() const { return Variables; }

  /// Run the dispatch command to completion.
  void run();

private:
  Device *Dev;          ///< The target device.
  const Module *Mod;    ///< The module containing the entry point to invoke.
  const Function *Func; ///< The entry point to invoke.
  Dim3 GroupSize;       ///< The size of each workgroup.
  Dim3 NumGroups;       ///< The number of workgroups.

  /// Index of next group to run in PendingGroups.
  size_t NextGroupIndex;

  /// Pool of group IDs pending creation and execution.
  std::vector<Dim3> PendingGroups;

  /// Pool of groups that have begun execution and been suspended.
  std::vector<Workgroup *> RunningGroups;

  Invocation *CurrentInvocation; ///< The current invocation being executed.
  Workgroup *CurrentGroup;       ///< The current workgroup being executed.

  VariableList Variables; ///< Resolved buffer variable values.

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
