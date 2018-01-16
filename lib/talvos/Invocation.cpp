// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <cassert>
#include <iostream>
#include <spirv/unified1/spirv.h>

#include "talvos/Invocation.h"
#include "talvos/Module.h"

namespace talvos
{

Invocation::Invocation(const Function *F)
{
  CurrentInstruction = F->FirstInstruction;
}

void Invocation::executeAccessChain(const Instruction *Inst)
{
  // TODO: Implement
  std::cout << "Executing OpAccessChain" << std::endl;
}

void Invocation::executeLoad(const Instruction *Inst)
{
  // TODO: Implement
  std::cout << "Executing OpLoad" << std::endl;
}

void Invocation::executeStore(const Instruction *Inst)
{
  // TODO: Implement
  std::cout << "Executing OpStore" << std::endl;
}

Invocation::State Invocation::getState() const
{
  return CurrentInstruction ? READY : FINISHED;
}

void Invocation::step()
{
  assert(CurrentInstruction);

  // Dispatch instruction to handler method.
  switch (CurrentInstruction->Opcode)
  {
#define DISPATCH(Op, Func)                                                     \
  case Op:                                                                     \
    execute##Func(CurrentInstruction);                                         \
    break;

    DISPATCH(SpvOpAccessChain, AccessChain);
    DISPATCH(SpvOpLoad, Load);
    DISPATCH(SpvOpStore, Store);

#undef DISPATCH

  default:
    std::cout << "Unhandled opcode " << CurrentInstruction->Opcode << std::endl;
  }

  // TODO: Handle branch/call/ret
  CurrentInstruction = CurrentInstruction->Next;
}

} // namespace talvos
