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
class Module;
class Object;

/// Map from Descriptor/Binding pair to an address in memory.
typedef std::map<std::pair<uint32_t, uint32_t>, uint64_t> DescriptorSet;

/// This class is used to invoke compute kernels on a virtual device.
class DispatchCommand
{
public:
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

  DispatchCommand(const DispatchCommand &) = delete;
  DispatchCommand &operator=(const DispatchCommand &) = delete;

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

  VariableList Variables; ///< Resolved buffer variable values.
};

} // namespace talvos

#endif
