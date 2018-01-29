// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <cassert>
#include <iostream>
#include <spirv/unified1/spirv.h>

#include "talvos/Device.h"
#include "talvos/Function.h"
#include "talvos/Instruction.h"
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
  Objects = M->cloneObjects();

  CurrentFunction = F;
  moveToBlock(F->getEntryBlockId());

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

  // Set up private variables.
  for (PrivateVariableMap::value_type V : M->getPrivateVariables())
  {
    // Allocate and initialize variable in private memory.
    size_t NumBytes = V.second.Ty->getElementType()->getSize();
    size_t Address = PrivateMemory->allocate(NumBytes);
    assert(V.second.Initializer);
    Objects[V.second.Initializer].store(PrivateMemory, Address);
    Objects[V.first] = Object::create<size_t>(V.second.Ty, Address);
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
  Object &Base = Objects[Inst->Operands[2]];

  // TODO: Generate useful error message for this
  assert(Base.isSet() && "Invalid base pointer - missing descriptor set?");

  size_t Result = Base.get<size_t>();
  const Type *ElemType = Base.getType()->getElementType(0);

  // Loop over indices.
  for (int i = 3; i < Inst->NumOperands; i++)
  {
    // TODO: Handle indices of different sizes.
    uint32_t Idx = OP(i, uint32_t);
    Result += ElemType->getElementOffset(Idx);
    ElemType = ElemType->getElementType(Idx);
  }

  Objects[Inst->Operands[1]] = Object::create<size_t>(Inst->ResultType, Result);
}

void Invocation::executeBranch(const Instruction *Inst)
{
  moveToBlock(Inst->Operands[0]);
}

void Invocation::executeBranchConditional(const Instruction *Inst)
{
  bool Condition = OP(0, bool);
  moveToBlock(Inst->Operands[Condition ? 1 : 2]);
}

void Invocation::executeCompositeExtract(const Instruction *Inst)
{
  uint32_t Id = Inst->Operands[1];
  // TODO: Handle indices of different sizes.
  std::vector<uint32_t> Indices(Inst->Operands + 3,
                                Inst->Operands + Inst->NumOperands);
  Objects[Id] = Objects[Inst->Operands[2]].extract(Indices);
}

void Invocation::executeIAdd(const Instruction *Inst)
{
  uint32_t Id = Inst->Operands[1];
  const Object &A = Objects[Inst->Operands[2]];
  const Object &B = Objects[Inst->Operands[3]];
  Object Result = Object::create(Inst->ResultType);
  for (int i = 0; i < Inst->ResultType->getElementCount(); i++)
  {
    // TODO: Use actual integer type
    Result.set<uint32_t>(A.get<uint32_t>(i) + B.get<uint32_t>(i), i);
  }
  Objects[Id] = Result;
}

void Invocation::executeIEqual(const Instruction *Inst)
{
  uint32_t Id = Inst->Operands[1];
  // TODO: Use actual integer type
  // TODO: Handle vectors
  bool Result = OP(2, uint32_t) == OP(3, uint32_t);
  Objects[Id] = Object::create<bool>(Inst->ResultType, Result);
}

void Invocation::executeLoad(const Instruction *Inst)
{
  uint32_t Id = Inst->Operands[1];
  const Object &Src = Objects[Inst->Operands[2]];
  Memory *Mem = getMemory(Src.getType()->getStorageClass());
  Objects[Id] = Object::load(Inst->ResultType, Mem, Src.get<size_t>());
}

void Invocation::executePhi(const Instruction *Inst)
{
  uint32_t Id = Inst->Operands[1];

  // TODO: Handle OpPhi as input to another OpPhi
  assert(PreviousBlock);
  for (int i = 2; i < Inst->NumOperands; i += 2)
  {
    assert(i + 1 < Inst->NumOperands);
    if (Inst->Operands[i + 1] == PreviousBlock)
    {
      Objects[Id] = Objects[Inst->Operands[i]].clone();
      return;
    }
  }
  assert(false && "no matching predecessor block for OpPhi");
}

void Invocation::executeReturn(const Instruction *Inst)
{
  // TODO: Jump back to callee function if necessary
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
  case SpvStorageClassPrivate:
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

void Invocation::moveToBlock(uint32_t Id)
{
  const Block *B = CurrentFunction->getBlock(Id);
  CurrentInstruction = B->FirstInstruction;
  PreviousBlock = CurrentBlock;
  CurrentBlock = Id;
}

void Invocation::step()
{
  assert(CurrentInstruction);

  const Instruction *I = CurrentInstruction;

  // Move program counter to next instruction.
  // Execution of terminator instruction may change this.
  CurrentInstruction = CurrentInstruction->Next;

  // Dispatch instruction to handler method.
  switch (I->Opcode)
  {
#define DISPATCH(Op, Func)                                                     \
  case Op:                                                                     \
    execute##Func(I);                                                          \
    break;
#define NOP(Op)                                                                \
  case Op:                                                                     \
    break;

    DISPATCH(SpvOpAccessChain, AccessChain);
    DISPATCH(SpvOpBranch, Branch);
    DISPATCH(SpvOpBranchConditional, BranchConditional);
    DISPATCH(SpvOpCompositeExtract, CompositeExtract);
    DISPATCH(SpvOpIAdd, IAdd);
    DISPATCH(SpvOpIEqual, IEqual);
    DISPATCH(SpvOpLoad, Load);
    DISPATCH(SpvOpPhi, Phi);
    DISPATCH(SpvOpReturn, Return);
    DISPATCH(SpvOpStore, Store);

    NOP(SpvOpLoopMerge);
    NOP(SpvOpSelectionMerge);

#undef DISPATCH
#undef NOP

  default:
    std::cout << "Unhandled opcode " << I->Opcode << std::endl;
  }
}

} // namespace talvos
