// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <cassert>
#include <iostream>
#include <spirv/unified1/spirv.h>

#include "talvos/Invocation.h"
#include "talvos/Module.h"

#define OP(Index, Type) Results[Inst->Operands[Index]].get<Type>()

namespace talvos
{

Invocation::Invocation(const Module *M, const Function *F)
{
  CurrentInstruction = F->FirstInstruction;
  Results = M->cloneResults();
}

Invocation::~Invocation()
{
  for (Result &R : Results)
    R.destroy();
}

void Invocation::executeAccessChain(const Instruction *Inst)
{
  // TODO: Implement
  Results[Inst->Operands[1]] = Result::create<void *>(nullptr);

  std::cout << "Executing OpAccessChain" << std::endl;
  for (int i = 3; i < Inst->NumOperands; i++)
  {
    uint32_t Idx = OP(i, uint32_t);
    std::cout << " - Index " << (i - 3) << " = " << Idx << std::endl;
  }
}

void Invocation::executeLoad(const Instruction *Inst)
{
  void *Src = OP(2, void *);

  // TODO: Load actual data
  Results[Inst->Operands[1]] = Result::create<uint32_t>(42);

  std::cout << "Load from " << Src << std::endl;
}

void Invocation::executeStore(const Instruction *Inst)
{
  void *Dest = OP(0, void *);

  // TODO: Store actual data
  uint32_t Obj = Results[Inst->Operands[1]].get<uint32_t>();

  std::cout << "Store " << Obj << " to " << Dest << std::endl;
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
