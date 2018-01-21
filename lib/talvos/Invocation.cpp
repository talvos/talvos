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
#include "talvos/Type.h"

#define OP(Index, Type) Objects[Inst->Operands[Index]].get<Type>()

namespace talvos
{

Invocation::Invocation(
    Device *D, const Module *M, const Function *F, uint32_t GroupIdX,
    uint32_t GroupIdY, uint32_t GroupIdZ,
    const std::vector<std::pair<uint32_t, Object>> &Variables)
{
  Dev = D;
  PrivateMemory = new Memory;
  CurrentInstruction = F->FirstInstruction;
  Objects = M->cloneObjects();

  // TODO: Handle local size larger than 1
  GlobalId[0] = GroupIdX;
  GlobalId[1] = GroupIdY;
  GlobalId[2] = GroupIdZ;

  // Copy variable pointer values.
  for (auto V : Variables)
    Objects[V.first] = V.second.clone();

  // Set up input variables.
  for (InputVariableMap::value_type V : M->getInputVariables())
  {
    switch (V.second.Builtin)
    {
    case SpvBuiltInGlobalInvocationId:
    {
      // Allocate and initialize global ID.
      size_t Address = PrivateMemory->allocate(sizeof(GlobalId));
      PrivateMemory->store(Address, sizeof(GlobalId), (uint8_t *)GlobalId);
      Objects[V.first] = Object::create<size_t>(V.second.Ty, Address);
      break;
    }
    default:
      std::cout << "Unhandled input variable builtin: " << V.second.Builtin
                << std::endl;
    }
  }
}

Invocation::~Invocation()
{
  for (Object &Obj : Objects)
    Obj.destroy();
  delete PrivateMemory;
}

void Invocation::executeAccessChain(const Instruction *Inst)
{
  // Base pointer.
  size_t Result = OP(2, size_t);

  // Loop over indices.
  for (int i = 3; i < Inst->NumOperands; i++)
  {
    // TODO: Handle indices of different sizes.
    uint32_t Idx = OP(i, uint32_t);
    // TODO: Use actual type.
    Result += Idx * sizeof(uint32_t);
  }

  Objects[Inst->Operands[1]] = Object::create<size_t>(Inst->ResultType, Result);
}

void Invocation::executeLoad(const Instruction *Inst)
{
  uint32_t Id = Inst->Operands[1];
  const Object &Src = Objects[Inst->Operands[2]];
  Memory *Mem = getMemory(Src.getType()->getStorageClass());
  Objects[Id] = Object::load(Inst->ResultType, Mem, Src.get<size_t>());
}

void Invocation::executeStore(const Instruction *Inst)
{
  uint32_t Id = Inst->Operands[1];
  const Object &Dest = Objects[Inst->Operands[0]];
  Memory *Mem = getMemory(Dest.getType()->getStorageClass());
  Objects[Id].store(Mem, Dest.get<size_t>());
}

Memory *Invocation::getMemory(uint32_t StorageClass)
{
  switch (StorageClass)
  {
  case SpvStorageClassStorageBuffer:
    return Dev->getGlobalMemory();
  case SpvStorageClassInput:
    return PrivateMemory;
  default:
    assert(false && "Unhandled storage class");
    return nullptr;
  }
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
