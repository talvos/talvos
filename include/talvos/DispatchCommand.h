// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file DispatchCommand.h
/// This file declares the DispatchCommand class.

#ifndef TALVOS_DISPATCHCOMMAND_H
#define TALVOS_DISPATCHCOMMAND_H

#include <vector>

#include "talvos/DescriptorSet.h"
#include "talvos/Dim3.h"

namespace talvos
{

class Function;
class Module;
class Object;

/// This class encapsulates information about a compute kernel launch.
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
  /// \param M The module containing the entry point to invoke.
  /// \param F The entry point to invoke.
  /// \param NumGroups The number of groups to launch.
  /// \param DSM The descriptor set mapping to use.
  DispatchCommand(const Module *M, const Function *F, Dim3 NumGroups,
                  const DescriptorSetMap &DSM);

  // Do not allow DispatchCommand objects to be copied.
  ///\{
  DispatchCommand(const DispatchCommand &) = delete;
  DispatchCommand &operator=(const DispatchCommand &) = delete;
  ///\}

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

private:
  const Module *Mod;    ///< The module containing the entry point to invoke.
  const Function *Func; ///< The entry point to invoke.
  Dim3 GroupSize;       ///< The size of each workgroup.
  Dim3 NumGroups;       ///< The number of workgroups.

  VariableList Variables; ///< Resolved buffer variable values.
};

} // namespace talvos

#endif
