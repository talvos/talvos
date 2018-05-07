// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Workgroup.h
/// This file declares the Workgroup class.

#ifndef TALVOS_WORKGROUP_H
#define TALVOS_WORKGROUP_H

#include <memory>
#include <vector>

#include "talvos/Dim3.h"

namespace talvos
{

class Device;
class Invocation;
class Memory;
class Object;
class ShaderExecution;

/// This class represents a workgroup executing a compute command.
class Workgroup
{
public:
  /// List of work items in the workgroup.
  typedef std::vector<std::unique_ptr<Invocation>> WorkItemList;

  /// List of mappings from variable ID to object.
  typedef std::vector<std::pair<uint32_t, Object>> VariableList;

  /// Create a workgroup.
  Workgroup(Device &Dev, const ShaderExecution &Execution, Dim3 GroupId);

  /// Destroy this workgroup.
  ~Workgroup();

  // Do not allow Workgroup objects to be copied.
  ///\{
  Workgroup(const Workgroup &) = delete;
  Workgroup &operator=(const Workgroup &) = delete;
  ///\}

  /// Returns the group ID of this workgroup.
  Dim3 getGroupId() const { return GroupId; }

  /// Returns the local memory instance associated with this workgroup.
  Memory &getLocalMemory() { return *LocalMemory; }

  /// Return the list of work items in this workgroup.
  const WorkItemList &getWorkItems() const { return WorkItems; }

  /// Return the workgroup scope variable pointer values.
  const VariableList &getVariables() const { return Variables; }

private:
  Dim3 GroupId; ///< The group ID.

  Memory *LocalMemory; ///< The local memory of this workgroup.

  WorkItemList WorkItems; ///< List of work items in this workgroup.

  VariableList Variables; ///< Workgroup scope OpVariable allocations.
};

} // namespace talvos

#endif
