// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <cassert>
#include <iostream>
#include <spirv/unified1/GLSL.std.450.h>
#include <spirv/unified1/spirv.h>

#include "talvos/Block.h"
#include "talvos/Device.h"
#include "talvos/DispatchCommand.h"
#include "talvos/Function.h"
#include "talvos/Instruction.h"
#include "talvos/Invocation.h"
#include "talvos/Memory.h"
#include "talvos/Module.h"
#include "talvos/Type.h"
#include "talvos/Workgroup.h"

#define OP(Index, Type) Objects[Inst->Operands[Index]].get<Type>()

namespace talvos
{

Invocation::Invocation(
    const DispatchCommand *Dispatch, Workgroup &Group, Dim3 LocalId,
    const std::vector<std::pair<uint32_t, Object>> &Variables)
    : Group(Group)
{
  Dev = Dispatch->getDevice();
  PrivateMemory = new Memory;

  CurrentModule = Dispatch->getModule();
  CurrentFunction = Dispatch->getFunction();
  moveToBlock(CurrentFunction->getFirstBlockId());

  // Set up the local and global ID.
  Dim3 GroupSize = Dispatch->getGroupSize();
  Dim3 NumGroups = Dispatch->getNumGroups();
  GroupId = Group.getGroupId();
  LocalId = LocalId;
  GlobalId = LocalId + GroupId * GroupSize;

  // Clone module level objects.
  Objects = CurrentModule->getObjects();

  // Copy variable pointer values.
  for (auto V : Variables)
    Objects[V.first] = V.second;

  // Set up input variables.
  for (InputVariableMap::value_type V : CurrentModule->getInputVariables())
  {
    // Get initialization data.
    size_t Sz;
    uint8_t *Data;
    switch (V.second.Builtin)
    {
    case SpvBuiltInGlobalInvocationId:
      Sz = sizeof(GlobalId);
      Data = (uint8_t *)GlobalId.Data;
      break;
    case SpvBuiltInLocalInvocationId:
      Sz = sizeof(LocalId);
      Data = (uint8_t *)LocalId.Data;
      break;
    case SpvBuiltInNumWorkgroups:
      Sz = sizeof(NumGroups);
      Data = (uint8_t *)NumGroups.Data;
      break;
    case SpvBuiltInWorkgroupId:
      Sz = sizeof(GroupId);
      Data = (uint8_t *)GroupId.Data;
      break;
    default:
      std::cerr << "Unimplemented input variable builtin: " << V.second.Builtin
                << std::endl;
      abort();
    }

    // Perform allocation and initialize it.
    uint64_t Address = PrivateMemory->allocate(Sz);
    PrivateMemory->store(Address, Sz, Data);
    Objects[V.first] = Object(V.second.Ty, Address);
  }

  // Set up private variables.
  for (PrivateVariableMap::value_type V : CurrentModule->getPrivateVariables())
  {
    // Allocate and initialize variable in private memory.
    uint64_t NumBytes = V.second.Ty->getElementType()->getSize();
    uint64_t Address = PrivateMemory->allocate(NumBytes);
    assert(V.second.Initializer);
    Objects[V.second.Initializer].store(*PrivateMemory, Address);
    Objects[V.first] = Object(V.second.Ty, Address);
  }
}

Invocation::~Invocation()
{
  delete PrivateMemory;
}

void Invocation::executeAccessChain(const Instruction *Inst)
{
  // Base pointer.
  Object &Base = Objects[Inst->Operands[2]];

  // TODO: Generate useful error message for this
  assert(Base && "Invalid base pointer - missing descriptor set?");

  uint64_t Result = Base.get<uint64_t>();
  const Type *ElemType = Base.getType()->getElementType(0);

  // Loop over indices.
  for (int i = 3; i < Inst->NumOperands; i++)
  {
    // TODO: Handle indices of different sizes.
    uint32_t Idx = OP(i, uint32_t);
    Result += ElemType->getElementOffset(Idx);
    ElemType = ElemType->getElementType(Idx);
  }

  Objects[Inst->Operands[1]] = Object(Inst->ResultType, Result);
}

template <typename OperandType, typename F>
void Invocation::executeBinaryOp(const Instruction *Inst, const F &Op)
{
  uint32_t Id = Inst->Operands[1];
  const Object &OpA = Objects[Inst->Operands[2]];
  const Object &OpB = Objects[Inst->Operands[3]];
  Object Result(Inst->ResultType);
  for (uint32_t i = 0; i < Inst->ResultType->getElementCount(); i++)
  {
    Result.set(Op(OpA.get<OperandType>(i), OpB.get<OperandType>(i)), i);
  }
  Objects[Id] = Result;
}

template <typename F>
void Invocation::executeBinaryOpSInt(const Instruction *Inst, const F &&Op)
{
  const Type *OpType = Objects[Inst->Operands[2]].getType()->getScalarType();
  assert(OpType->isInt());
  switch (OpType->getBitWidth())
  {
  case 16:
    executeBinaryOp<int16_t>(Inst, Op);
    break;
  case 32:
    executeBinaryOp<int32_t>(Inst, Op);
    break;
  case 64:
    executeBinaryOp<int64_t>(Inst, Op);
    break;
  default:
    assert(false && "Unhandled binary operation integer width");
  }
}

template <typename F>
void Invocation::executeBinaryOpFP(const Instruction *Inst, const F &&Op)
{
  const Type *OpType = Objects[Inst->Operands[2]].getType()->getScalarType();
  assert(OpType->isFloat());
  switch (OpType->getBitWidth())
  {
  case 32:
    executeBinaryOp<float>(Inst, Op);
    break;
  case 64:
    executeBinaryOp<double>(Inst, Op);
    break;
  default:
    assert(false && "Unhandled binary operation floating point size");
  }
}

template <typename F>
void Invocation::executeBinaryOpUInt(const Instruction *Inst, const F &&Op)
{
  const Type *OpType = Objects[Inst->Operands[2]].getType()->getScalarType();
  assert(OpType->isInt());
  switch (OpType->getBitWidth())
  {
  case 16:
    executeBinaryOp<uint16_t>(Inst, Op);
    break;
  case 32:
    executeBinaryOp<uint32_t>(Inst, Op);
    break;
  case 64:
    executeBinaryOp<uint64_t>(Inst, Op);
    break;
  default:
    assert(false && "Unhandled binary operation integer width");
  }
}

void Invocation::executeBitwiseAnd(const Instruction *Inst)
{
  executeBinaryOpUInt(Inst,
                      [](auto A, auto B) -> decltype(A) { return A & B; });
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

void Invocation::executeCompositeInsert(const Instruction *Inst)
{
  uint32_t Id = Inst->Operands[1];
  Object &Element = Objects[Inst->Operands[2]];
  // TODO: Handle indices of different sizes.
  std::vector<uint32_t> Indices(Inst->Operands + 4,
                                Inst->Operands + Inst->NumOperands);
  assert(Objects[Inst->Operands[3]].getType()->isComposite());
  Objects[Id] = Objects[Inst->Operands[3]];
  Objects[Id].insert(Indices, Element);
}

void Invocation::executeExtInst(const Instruction *Inst)
{
  // TODO: Currently assumes extended instruction set is GLSL.std.450
  // TODO: Use dispatch mechanism similar to step()?
  uint32_t Id = Inst->Operands[1];
  uint32_t ExtInst = Inst->Operands[3];
  switch (ExtInst)
  {
  case GLSLstd450Fma:
    // TODO: Handle vectors and double precision (using a helper function?)
    assert(Inst->ResultType->isFloat());
    assert(Inst->ResultType->getBitWidth() == 32);
    Objects[Id] =
        Object(Inst->ResultType, OP(4, float) * OP(5, float) + OP(6, float));
    break;
  default:
    assert(false && "Unhandled GLSL.std.450 extended instruction");
  }
}

void Invocation::executeFAdd(const Instruction *Inst)
{
  executeBinaryOpFP(Inst, [](auto A, auto B) -> decltype(A) { return A + B; });
}

void Invocation::executeFDiv(const Instruction *Inst)
{
  executeBinaryOpFP(Inst, [](auto A, auto B) -> decltype(A) { return A / B; });
}

void Invocation::executeFMul(const Instruction *Inst)
{
  executeBinaryOpFP(Inst, [](auto A, auto B) -> decltype(A) { return A * B; });
}

void Invocation::executeFSub(const Instruction *Inst)
{
  executeBinaryOpFP(Inst, [](auto A, auto B) -> decltype(A) { return A - B; });
}

void Invocation::executeFunctionCall(const Instruction *Inst)
{
  const Function *Func = CurrentModule->getFunction(Inst->Operands[2]);

  // Copy function parameters.
  assert(Func->getNumParams() == Inst->NumOperands - 3);
  for (int i = 3; i < Inst->NumOperands; i++)
    Objects[Func->getParamId(i - 3)] = Objects[Inst->Operands[i]];

  // Create call stack entry.
  StackEntry SE;
  SE.RetInst = CurrentInstruction;
  SE.RetFunc = CurrentFunction;
  SE.RetBlock = CurrentBlock;
  CallStack.push_back(SE);

  // Move to first block of callee function.
  CurrentFunction = Func;
  moveToBlock(CurrentFunction->getFirstBlockId());
}

void Invocation::executeIAdd(const Instruction *Inst)
{
  executeBinaryOpUInt(Inst,
                      [](auto A, auto B) -> decltype(A) { return A + B; });
}

void Invocation::executeIEqual(const Instruction *Inst)
{
  executeBinaryOpUInt(Inst, [](auto &&A, auto &&B) -> bool { return A == B; });
}

void Invocation::executeIMul(const Instruction *Inst)
{
  executeBinaryOpUInt(Inst,
                      [](auto A, auto B) -> decltype(A) { return A * B; });
}

void Invocation::executeLoad(const Instruction *Inst)
{
  uint32_t Id = Inst->Operands[1];
  const Object &Src = Objects[Inst->Operands[2]];
  Memory &Mem = getMemory(Src.getType()->getStorageClass());
  Objects[Id] = Object::load(Inst->ResultType, Mem, Src.get<uint64_t>());
}

void Invocation::executeLogicalNot(const Instruction *Inst)
{
  executeUnaryOp<bool>(Inst, [](bool &&A) { return !A; });
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
      Objects[Id] = Objects[Inst->Operands[i]];
      return;
    }
  }
  assert(false && "no matching predecessor block for OpPhi");
}

void Invocation::executePtrAccessChain(const Instruction *Inst)
{
  // Base pointer.
  Object &Base = Objects[Inst->Operands[2]];

  // TODO: Generate useful error message for this
  assert(Base && "Invalid base pointer - missing descriptor set?");

  uint64_t Result = Base.get<uint64_t>();
  const Type *ElemType = Base.getType()->getElementType();

  // Perform initial deference for element index.
  Result += Base.getType()->getElementOffset(OP(3, uint32_t));

  // Loop over indices.
  for (int i = 4; i < Inst->NumOperands; i++)
  {
    // TODO: Handle indices of different sizes.
    uint32_t Idx = OP(i, uint32_t);
    Result += ElemType->getElementOffset(Idx);
    ElemType = ElemType->getElementType(Idx);
  }

  Objects[Inst->Operands[1]] = Object(Inst->ResultType, Result);
}

void Invocation::executeReturn(const Instruction *Inst)
{
  // If this is the entry function, do nothing.
  if (CallStack.empty())
    return;

  StackEntry SE = CallStack.back();
  CallStack.pop_back();

  // Release function scope allocations.
  for (uint64_t Address : SE.Allocations)
    PrivateMemory->release(Address);

  // Return to callee function.
  CurrentFunction = SE.RetFunc;
  CurrentBlock = SE.RetBlock;
  CurrentInstruction = SE.RetInst;
}

void Invocation::executeReturnValue(const Instruction *Inst)
{
  assert(!CallStack.empty());

  StackEntry SE = CallStack.back();
  CallStack.pop_back();

  // Set return value.
  Objects[SE.RetInst->Operands[1]] = Objects[Inst->Operands[0]];

  // Release function scope allocations.
  for (uint64_t Address : SE.Allocations)
    PrivateMemory->release(Address);

  // Return to callee function.
  CurrentFunction = SE.RetFunc;
  CurrentBlock = SE.RetBlock;
  CurrentInstruction = SE.RetInst;
}

void Invocation::executeSGreaterThan(const Instruction *Inst)
{
  executeBinaryOpSInt(Inst, [](auto &&A, auto &&B) -> bool { return A > B; });
}

void Invocation::executeSLessThan(const Instruction *Inst)
{
  executeBinaryOpSInt(Inst, [](auto &&A, auto &&B) -> bool { return A < B; });
}

void Invocation::executeStore(const Instruction *Inst)
{
  uint32_t Id = Inst->Operands[1];
  const Object &Dest = Objects[Inst->Operands[0]];
  Memory &Mem = getMemory(Dest.getType()->getStorageClass());
  Objects[Id].store(Mem, Dest.get<uint64_t>());
}

void Invocation::executeULessThan(const Instruction *Inst)
{
  executeBinaryOpUInt(Inst, [](auto &&A, auto &&B) -> bool { return A < B; });
}

template <typename OperandType, typename F>
void Invocation::executeUnaryOp(const Instruction *Inst, const F &Op)
{
  uint32_t Id = Inst->Operands[1];
  const Object &OpA = Objects[Inst->Operands[2]];
  Object Result(Inst->ResultType);
  for (uint32_t i = 0; i < Inst->ResultType->getElementCount(); i++)
  {
    Result.set(Op(OpA.get<OperandType>(i)), i);
  }
  Objects[Id] = Result;
}

void Invocation::executeUndef(const Instruction *Inst)
{
  Objects[Inst->Operands[1]] = Object(Inst->ResultType);
}

void Invocation::executeVariable(const Instruction *Inst)
{
  assert(Inst->Operands[2] == SpvStorageClassFunction);

  uint32_t Id = Inst->Operands[1];
  size_t AllocSize = Inst->ResultType->getElementType()->getSize();
  uint64_t Address = PrivateMemory->allocate(AllocSize);
  Objects[Id] = Object(Inst->ResultType, Address);

  // Track function scope allocations.
  if (!CallStack.empty())
    CallStack.back().Allocations.push_back(Address);
}

Memory &Invocation::getMemory(uint32_t StorageClass)
{
  switch (StorageClass)
  {
  case SpvStorageClassStorageBuffer:
    return Dev->getGlobalMemory();
  case SpvStorageClassWorkgroup:
    return Group.getLocalMemory();
  case SpvStorageClassInput:
  case SpvStorageClassFunction:
  case SpvStorageClassPrivate:
    return *PrivateMemory;
  default:
    assert(false && "Unhandled storage class");
    abort();
  }
}

Invocation::State Invocation::getState() const
{
  return CurrentInstruction ? READY : FINISHED;
}

void Invocation::moveToBlock(uint32_t Id)
{
  const Block *B = CurrentFunction->getBlock(Id);
  CurrentInstruction = B->getFirstInstruction();
  PreviousBlock = CurrentBlock;
  CurrentBlock = Id;
}

void Invocation::step()
{
  assert(CurrentInstruction);

  const Instruction *I = CurrentInstruction;

  // Move program counter to next instruction.
  // Execution of terminator instruction may change this.
  CurrentInstruction = CurrentInstruction->next();

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
    DISPATCH(SpvOpBitwiseAnd, BitwiseAnd);
    DISPATCH(SpvOpBranch, Branch);
    DISPATCH(SpvOpBranchConditional, BranchConditional);
    DISPATCH(SpvOpCompositeExtract, CompositeExtract);
    DISPATCH(SpvOpCompositeInsert, CompositeInsert);
    DISPATCH(SpvOpExtInst, ExtInst);
    DISPATCH(SpvOpFAdd, FAdd);
    DISPATCH(SpvOpFDiv, FDiv);
    DISPATCH(SpvOpFMul, FMul);
    DISPATCH(SpvOpFSub, FSub);
    DISPATCH(SpvOpFunctionCall, FunctionCall);
    DISPATCH(SpvOpIAdd, IAdd);
    DISPATCH(SpvOpIEqual, IEqual);
    DISPATCH(SpvOpIMul, IMul);
    DISPATCH(SpvOpLoad, Load);
    DISPATCH(SpvOpLogicalNot, LogicalNot);
    DISPATCH(SpvOpPhi, Phi);
    DISPATCH(SpvOpPtrAccessChain, PtrAccessChain);
    DISPATCH(SpvOpReturn, Return);
    DISPATCH(SpvOpReturnValue, ReturnValue);
    DISPATCH(SpvOpSGreaterThan, SGreaterThan);
    DISPATCH(SpvOpSLessThan, SLessThan);
    DISPATCH(SpvOpStore, Store);
    DISPATCH(SpvOpULessThan, ULessThan);
    DISPATCH(SpvOpUndef, Undef);
    DISPATCH(SpvOpVariable, Variable);

    NOP(SpvOpLoopMerge);
    NOP(SpvOpSelectionMerge);

#undef DISPATCH
#undef NOP

  default:
    std::cerr << "Unimplemented opcode " << I->Opcode << std::endl;
    abort();
  }
}

} // namespace talvos
