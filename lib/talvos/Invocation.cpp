// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <array>
#include <cassert>
#include <cmath>
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

Invocation::Invocation(const DispatchCommand *Dispatch, Workgroup *Group,
                       Dim3 LocalId)
{
  Dev = Dispatch->getDevice();
  PrivateMemory = new Memory;
  this->Group = Group;

  AtBarrier = false;
  CurrentModule = Dispatch->getModule();
  CurrentFunction = Dispatch->getFunction();
  moveToBlock(CurrentFunction->getFirstBlockId());

  // Set up the local and global ID.
  Dim3 GroupSize = Dispatch->getGroupSize();
  Dim3 NumGroups = Dispatch->getNumGroups();
  GroupId = Group->getGroupId();
  LocalId = LocalId;
  GlobalId = LocalId + GroupId * GroupSize;

  // Clone module level objects.
  Objects = CurrentModule->getObjects();

  // Copy buffer variable pointer values.
  for (auto V : Dispatch->getVariables())
    Objects[V.first] = V.second;

  // Copy workgroup variable pointer values.
  for (auto V : Group->getVariables())
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

void Invocation::executeBitwiseAnd(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A & B; });
}

void Invocation::executeBitwiseOr(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A | B; });
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

void Invocation::executeControlBarrier(const Instruction *Inst)
{
  // TODO: Handle other execution scopes
  assert(Objects[Inst->Operands[0]].get<uint32_t>() == SpvScopeWorkgroup);
  AtBarrier = true;
}

void Invocation::executeExtInst(const Instruction *Inst)
{
  // TODO: Currently assumes extended instruction set is GLSL.std.450
  uint32_t ExtInst = Inst->Operands[3];
  switch (ExtInst)
  {
  case GLSLstd450Acos:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return acos(X); });
    break;
  case GLSLstd450Acosh:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return acosh(X); });
    break;
  case GLSLstd450Asin:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return asin(X); });
    break;
  case GLSLstd450Asinh:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return asinh(X); });
    break;
  case GLSLstd450Atan:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return atan(X); });
    break;
  case GLSLstd450Atanh:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return atanh(X); });
    break;
  case GLSLstd450Atan2:
    executeOpFP<2, 4>(
        Inst, [](auto Y, auto X) -> decltype(X) { return atan2(Y, X); });
    break;
  case GLSLstd450Cos:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return cos(X); });
    break;
  case GLSLstd450Cosh:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return cosh(X); });
    break;
  case GLSLstd450Fma:
  {
    executeOpFP<3, 4>(
        Inst, [](auto A, auto B, auto C) -> decltype(A) { return A * B + C; });
    break;
  }
  case GLSLstd450InverseSqrt:
    executeOpFP<1, 4>(Inst,
                      [](auto X) -> decltype(X) { return 1.f / sqrt(X); });
    break;
  case GLSLstd450Sin:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return sin(X); });
    break;
  case GLSLstd450Sinh:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return sinh(X); });
    break;
  case GLSLstd450Tan:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return tan(X); });
    break;
  case GLSLstd450Tanh:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return tanh(X); });
    break;
  default:
    assert(false && "Unhandled GLSL.std.450 extended instruction");
  }
}

void Invocation::executeFAdd(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> decltype(A) { return A + B; });
}

void Invocation::executeFDiv(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> decltype(A) { return A / B; });
}

void Invocation::executeFMul(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> decltype(A) { return A * B; });
}

void Invocation::executeFSub(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> decltype(A) { return A - B; });
}

void Invocation::executeFunctionCall(const Instruction *Inst)
{
  const Function *Func = CurrentModule->getFunction(Inst->Operands[2]);

  // Copy function parameters.
  assert(Inst->NumOperands == Func->getNumParams() + 3);
  for (int i = 3; i < Inst->NumOperands; i++)
    Objects[Func->getParamId(i - 3)] = Objects[Inst->Operands[i]];

  // Create call stack entry.
  StackEntry SE;
  SE.RetInst = Inst;
  SE.RetFunc = CurrentFunction;
  SE.RetBlock = CurrentBlock;
  CallStack.push_back(SE);

  // Move to first block of callee function.
  CurrentFunction = Func;
  moveToBlock(CurrentFunction->getFirstBlockId());
}

void Invocation::executeIAdd(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A + B; });
}

void Invocation::executeIEqual(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> bool { return A == B; });
}

void Invocation::executeIMul(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A * B; });
}

void Invocation::executeINotEqual(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> bool { return A != B; });
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
  executeOp<bool, 1>(Inst, [](bool A) { return !A; });
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
  CurrentInstruction = SE.RetInst->next();
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
  CurrentInstruction = SE.RetInst->next();
}

void Invocation::executeSGreaterThan(const Instruction *Inst)
{
  executeOpSInt<2>(Inst, [](auto A, auto B) -> bool { return A > B; });
}

void Invocation::executeShiftRightLogical(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A >> B; });
}

void Invocation::executeSLessThan(const Instruction *Inst)
{
  executeOpSInt<2>(Inst, [](auto A, auto B) -> bool { return A < B; });
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
  executeOpUInt<2>(Inst, [](auto A, auto B) -> bool { return A < B; });
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

void Invocation::executeVectorShuffle(const Instruction *Inst)
{
  uint32_t Id = Inst->Operands[1];
  Object Result(Inst->ResultType);

  const Object &Vec1 = Objects[Inst->Operands[2]];
  const Object &Vec2 = Objects[Inst->Operands[3]];
  uint32_t Vec1Length = Vec1.getType()->getElementCount();

  for (uint32_t i = 0; i < Inst->ResultType->getElementCount(); i++)
  {
    uint32_t Idx = Inst->Operands[4 + i];
    if (Idx < Vec1Length)
      Result.insert({i}, Vec1.extract({Idx}));
    else
      Result.insert({i}, Vec2.extract({Idx - Vec1Length}));
  }

  Objects[Id] = Result;
}

Memory &Invocation::getMemory(uint32_t StorageClass)
{
  switch (StorageClass)
  {
  case SpvStorageClassStorageBuffer:
    return Dev->getGlobalMemory();
  case SpvStorageClassWorkgroup:
    assert(Group && "Not executing within a workgroup.");
    return Group->getLocalMemory();
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
  if (AtBarrier)
    return BARRIER;
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
  assert(getState() == READY);
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
    DISPATCH(SpvOpBitwiseOr, BitwiseOr);
    DISPATCH(SpvOpBranch, Branch);
    DISPATCH(SpvOpBranchConditional, BranchConditional);
    DISPATCH(SpvOpCompositeExtract, CompositeExtract);
    DISPATCH(SpvOpCompositeInsert, CompositeInsert);
    DISPATCH(SpvOpControlBarrier, ControlBarrier);
    DISPATCH(SpvOpExtInst, ExtInst);
    DISPATCH(SpvOpFAdd, FAdd);
    DISPATCH(SpvOpFDiv, FDiv);
    DISPATCH(SpvOpFMul, FMul);
    DISPATCH(SpvOpFSub, FSub);
    DISPATCH(SpvOpFunctionCall, FunctionCall);
    DISPATCH(SpvOpIAdd, IAdd);
    DISPATCH(SpvOpIEqual, IEqual);
    DISPATCH(SpvOpIMul, IMul);
    DISPATCH(SpvOpINotEqual, INotEqual);
    DISPATCH(SpvOpLoad, Load);
    DISPATCH(SpvOpLogicalNot, LogicalNot);
    DISPATCH(SpvOpPhi, Phi);
    DISPATCH(SpvOpPtrAccessChain, PtrAccessChain);
    DISPATCH(SpvOpReturn, Return);
    DISPATCH(SpvOpReturnValue, ReturnValue);
    DISPATCH(SpvOpSGreaterThan, SGreaterThan);
    DISPATCH(SpvOpShiftRightLogical, ShiftRightLogical);
    DISPATCH(SpvOpSLessThan, SLessThan);
    DISPATCH(SpvOpStore, Store);
    DISPATCH(SpvOpULessThan, ULessThan);
    DISPATCH(SpvOpUndef, Undef);
    DISPATCH(SpvOpVariable, Variable);
    DISPATCH(SpvOpVectorShuffle, VectorShuffle);

    NOP(SpvOpLoopMerge);
    NOP(SpvOpSelectionMerge);

#undef DISPATCH
#undef NOP

  default:
    std::cerr << "Unimplemented opcode " << I->Opcode << std::endl;
    abort();
  }
}

// Private helper functions for executing simple instructions.

template <typename OpTy, typename F>
static auto apply(const std::array<OpTy, 1> Operands, const F &Op)
{
  return Op(Operands[0]);
}

template <typename OpTy, typename F>
static auto apply(const std::array<OpTy, 2> Operands, const F &Op)
{
  return Op(Operands[0], Operands[1]);
}

template <typename OpTy, typename F>
static auto apply(const std::array<OpTy, 3> Operands, const F &Op)
{
  return Op(Operands[0], Operands[1], Operands[2]);
}

template <typename OpTy, unsigned N, unsigned Offset, typename F>
void Invocation::executeOp(const Instruction *Inst, const F &Op)
{
  uint32_t Id = Inst->Operands[1];
  Object Result(Inst->ResultType);
  std::array<OpTy, N> Operands;

  // Loop over each vector component.
  for (uint32_t i = 0; i < Inst->ResultType->getElementCount(); i++)
  {
    // Gather operands.
    for (unsigned j = 0; j < N; j++)
      Operands[j] = Objects[Inst->Operands[Offset + j]].get<OpTy>(i);

    // Apply lambda and set result.
    Result.set(apply(Operands, Op), i);
  }

  Objects[Id] = Result;
}

template <unsigned N, unsigned Offset, typename F>
void Invocation::executeOpSInt(const Instruction *Inst, const F &&Op)
{
  const Type *OpType = Objects[Inst->Operands[Offset]].getType();
  OpType = OpType->getScalarType();
  assert(OpType->isInt());
  switch (OpType->getBitWidth())
  {
  case 16:
    executeOp<int16_t, N, Offset>(Inst, Op);
    break;
  case 32:
    executeOp<int32_t, N, Offset>(Inst, Op);
    break;
  case 64:
    executeOp<int64_t, N, Offset>(Inst, Op);
    break;
  default:
    assert(false && "Unhandled binary operation integer width");
  }
}

template <unsigned N, unsigned Offset, typename F>
void Invocation::executeOpFP(const Instruction *Inst, const F &&Op)
{
  const Type *OpType = Objects[Inst->Operands[Offset]].getType();
  OpType = OpType->getScalarType();
  assert(OpType->isFloat());
  switch (OpType->getBitWidth())
  {
  case 32:
    executeOp<float, N, Offset>(Inst, Op);
    break;
  case 64:
    executeOp<double, N, Offset>(Inst, Op);
    break;
  default:
    assert(false && "Unhandled binary operation floating point size");
  }
}

template <unsigned N, unsigned Offset, typename F>
void Invocation::executeOpUInt(const Instruction *Inst, const F &&Op)
{
  const Type *OpType = Objects[Inst->Operands[Offset]].getType();
  OpType = OpType->getScalarType();
  assert(OpType->isInt());
  switch (OpType->getBitWidth())
  {
  case 16:
    executeOp<uint16_t, N>(Inst, Op);
    break;
  case 32:
    executeOp<uint32_t, N>(Inst, Op);
    break;
  case 64:
    executeOp<uint64_t, N>(Inst, Op);
    break;
  default:
    assert(false && "Unhandled binary operation integer width");
  }
}

} // namespace talvos
