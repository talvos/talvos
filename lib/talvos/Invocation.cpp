// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <cassert>
#include <iostream>
#include <spirv/unified1/spirv.h>

#include "talvos/Device.h"
#include "talvos/Invocation.h"
#include "talvos/Memory.h"
#include "talvos/Module.h"

#define OP(Index, Type) Objects[Inst->Operands[Index]].get<Type>()

namespace talvos
{

Invocation::Invocation(
    Device *D, const Module *M, const Function *F,
    const std::vector<std::pair<uint32_t, size_t>> &Variables)
{
  Dev = D;
  CurrentInstruction = F->FirstInstruction;
  Objects = M->cloneObjects();

  // Copy variable pointer values.
  for (auto V : Variables)
    Objects[V.first] = Object::create<size_t>(V.second);
}

Invocation::~Invocation()
{
  for (Object &Obj : Objects)
    Obj.destroy();
}

void Invocation::executeAccessChain(const Instruction *Inst)
{
  // TODO: Implement
  Objects[Inst->Operands[1]] = Object::create<void *>(nullptr);

  std::cout << "Executing OpAccessChain" << std::endl;
  for (int i = 3; i < Inst->NumOperands; i++)
  {
    uint32_t Idx = OP(i, uint32_t);
    std::cout << " - Index " << (i - 3) << " = " << Idx << std::endl;
  }
}

void Invocation::executeLoad(const Instruction *Inst)
{
  uint32_t Id = Inst->Operands[1];
  size_t Src = OP(2, size_t);
  // TODO: Use result type
  Objects[Id] = Object::load(Dev->getGlobalMemory(), Src);
}

void Invocation::executeStore(const Instruction *Inst)
{
  uint32_t Id = Inst->Operands[1];
  size_t Dest = OP(0, size_t);
  Objects[Id].store(Dev->getGlobalMemory(), Dest);
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
