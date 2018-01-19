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

DispatchCommand::DispatchCommand(Device *D, const Module *M, const Function *F)
{
  Dev = D;
  Mod = M;
  Func = F;

  const VariableMap Variables = M->getVariables();
  for (VariableMap::value_type V : Variables)
  {
    // TODO: Set variable result to real value
    if (V.second.StorageClass == SpvStorageClassStorageBuffer)
    {
      std::cout << "Buffer variable %" << V.first
                << " with D/B = " << V.second.DescriptorSet << "/"
                << V.second.Binding << std::endl;
    }
    else
    {
      std::cout << "Variable %" << V.first
                << " with StorageClass = " << V.second.StorageClass
                << std::endl;
    }
  }
}

void DispatchCommand::run()
{
  // TODO: Launch more than one invocation
  Invocation I(Dev, Mod, Func);

  // TODO: Handle barriers
  while (I.getState() == Invocation::READY)
  {
    I.step();
  }
}

} // namespace talvos
