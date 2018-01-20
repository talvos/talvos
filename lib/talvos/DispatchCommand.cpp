// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <iostream>

#include <spirv/unified1/spirv.h>

#include "talvos/DispatchCommand.h"
#include "talvos/Invocation.h"
#include "talvos/Module.h"

namespace talvos
{

DispatchCommand::DispatchCommand(Device *D, const Module *M, const Function *F,
                                 const DescriptorSet &DS)
{
  Dev = D;
  Mod = M;
  Func = F;

  for (VariableMap::value_type V : M->getVariables())
  {
    if (V.second.StorageClass == SpvStorageClassStorageBuffer)
    {
      // Look-up variable in descriptor set and set pointer value.
      std::pair<size_t, size_t> Binding = {V.second.DescriptorSet,
                                           V.second.Binding};
      if (DS.count(Binding))
        Variables.push_back({V.first, DS.at(Binding)});
    }
    else
    {
      // TODO: Handle this
      std::cout << "Variable %" << V.first
                << " with StorageClass = " << V.second.StorageClass
                << std::endl;
    }
  }
}

void DispatchCommand::run()
{
  // TODO: Launch more than one invocation
  Invocation I(Dev, Mod, Func, Variables);

  // TODO: Handle barriers
  while (I.getState() == Invocation::READY)
  {
    I.step();
  }
}

} // namespace talvos
