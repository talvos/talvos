// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_WORKGROUP_H
#define TALVOS_WORKGROUP_H

#include "talvos/Dim3.h"

namespace talvos
{

class Memory;

/// This class represents a workgroup executing a compute command.
class Workgroup
{
public:
  /// Create a workgroup.
  Workgroup(Dim3 GroupId);

  /// Destroy this workgroup.
  ~Workgroup();

  // Do not allow Workgroups to be copied.
  Workgroup(const Workgroup &) = delete;
  Workgroup &operator=(const Workgroup &) = delete;

  /// Returns the group ID of this workgroup.
  Dim3 getGroupId() const { return GroupId; }

  /// Returns the local memory instance associated with this workgroup.
  Memory &getLocalMemory() { return *LocalMemory; }

private:
  Dim3 GroupId; ///< The group ID.

  Memory *LocalMemory; ///< The local memory of this workgroup.
};

} // namespace talvos

#endif
