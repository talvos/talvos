// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file DispatchCommand.cpp
/// This file defines the DispatchCommand class.

#include "talvos/DispatchCommand.h"
#include "talvos/Function.h"
#include "talvos/Module.h"

namespace talvos
{

DispatchCommand::DispatchCommand(const Module *M, const Function *F,
                                 Dim3 NumGroups, const DescriptorSet &DS)
{
  Mod = M;
  Func = F;

  this->NumGroups = NumGroups;
  this->GroupSize = M->getLocalSize(F->getId());
  // TODO: Handle WorkgroupSize decoration

  // Resolve buffer variables.
  for (BufferVariableMap::value_type V : M->getBufferVariables())
  {
    // Look up variable in descriptor set and set pointer value.
    std::pair<uint32_t, uint32_t> Binding = {V.second.DescriptorSet,
                                             V.second.Binding};
    if (DS.count(Binding))
    {
      Object Pointer(V.second.Ty, DS.at(Binding));
      Variables.push_back({V.first, Pointer});
    }
  }
}

} // namespace talvos
