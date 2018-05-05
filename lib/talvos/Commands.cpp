// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Commands.cpp
/// This file defines the Command class and its subclasses.

#include "talvos/Commands.h"
#include "talvos/Module.h"
#include "talvos/Pipeline.h"

namespace talvos
{

DispatchCommand::DispatchCommand(const Pipeline *P, Dim3 NumGroups,
                                 const DescriptorSetMap &DSM)
{
  PL = P;

  this->NumGroups = NumGroups;

  Objects = P->getObjects();

  // Resolve buffer variables.
  for (BufferVariableMap::value_type V : P->getModule()->getBufferVariables())
  {
    // Look up variable in descriptor set and set pointer value if present.
    uint32_t Set = V.second.DescriptorSet;
    uint32_t Binding = V.second.Binding;
    if (!DSM.count(Set))
      continue;
    if (!DSM.at(Set).count(Binding))
      continue;
    Objects[V.first] = Object(V.second.Ty, DSM.at(Set).at(Binding));
  }
}

} // namespace talvos
