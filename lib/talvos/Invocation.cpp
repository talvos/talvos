// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <iostream>

#include "talvos/Invocation.h"
#include "talvos/Module.h"

namespace talvos
{

Invocation::Invocation(const Function *F)
{
  CurrentInstruction = F->FirstInstruction;

  // TODO: Move this to run/step etc
  while (CurrentInstruction)
  {
    std::cout << "Executing " << CurrentInstruction->Opcode << std::endl;
    CurrentInstruction = CurrentInstruction->Next;
  }
}

} // namespace talvos
