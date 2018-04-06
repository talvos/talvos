// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Invocation.cpp
/// This file defines the Invocation class.

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>

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

/// Get scalar operand at index \p Index with type \p Type.
#define OP(Index, Type) Objects[Inst->getOperand(Index)].get<Type>()

namespace talvos
{

Invocation::Invocation(Device &Dev, const DispatchCommand &Command,
                       Workgroup *Group, Dim3 LocalId)
    : Dev(Dev)
{
  PrivateMemory = new Memory(Dev, MemoryScope::Invocation);
  this->Group = Group;

  AtBarrier = false;
  CurrentModule = Command.getModule();
  CurrentFunction = Command.getFunction();
  moveToBlock(CurrentFunction->getFirstBlockId());

  // Set up the local and global ID.
  Dim3 GroupSize = Command.getGroupSize();
  Dim3 NumGroups = Command.getNumGroups();
  this->LocalId = LocalId;
  GroupId = Group->getGroupId();
  GlobalId = LocalId + GroupId * GroupSize;

  // Clone module level objects.
  Objects = CurrentModule->getObjects();

  // Copy buffer variable pointer values.
  for (auto V : Command.getVariables())
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

  Dev.reportInvocationBegin(this);
}

Invocation::~Invocation() { delete PrivateMemory; }

void Invocation::executeAccessChain(const Instruction *Inst)
{
  // Base pointer.
  uint32_t Id = Inst->getOperand(1);
  Object &Base = Objects[Inst->getOperand(2)];

  // Ensure base pointer is valid.
  if (!Base)
  {
    // Check for buffer variable matching base pointer ID.
    for (BufferVariableMap::value_type V : CurrentModule->getBufferVariables())
    {
      if (V.first == Inst->getOperand(2))
      {
        // Report error for missing descriptor set entry.
        std::stringstream Err;
        Err << "Invalid base pointer for descriptor set entry ("
            << V.second.DescriptorSet << "," << V.second.Binding << ")";
        Dev.reportError(Err.str());

        // Set result pointer to null.
        Objects[Id] = Object(Inst->getResultType(), (uint64_t)0);
        return;
      }
    }
    assert(false && "Invalid base pointer for OpAccessChain");
  }

  uint64_t Result = Base.get<uint64_t>();
  const Type *ElemType = Base.getType()->getElementType(0);

  // Loop over indices.
  for (int i = 3; i < Inst->getNumOperands(); i++)
  {
    uint64_t Index;
    const Object &IndexObj = Objects[Inst->getOperand(i)];
    switch (IndexObj.getType()->getSize())
    {
    case 2:
      Index = IndexObj.get<uint16_t>();
      break;
    case 4:
      Index = IndexObj.get<uint32_t>();
      break;
    case 8:
      Index = IndexObj.get<uint64_t>();
      break;
    default:
      Dev.reportError("Unhandled index size", true);
      return;
    }
    Result += ElemType->getElementOffset(Index);
    ElemType = ElemType->getElementType(Index);
  }

  Objects[Id] = Object(Inst->getResultType(), Result);
}

void Invocation::executeBitcast(const Instruction *Inst)
{
  const Object &Source = Objects[Inst->getOperand(2)];
  Object Result = Object(Inst->getResultType(), Source.getData());
  Objects[Inst->getOperand(1)] = Result;
}

void Invocation::executeBitwiseAnd(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A & B; });
}

void Invocation::executeBitwiseOr(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A | B; });
}

void Invocation::executeBitwiseXor(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A ^ B; });
}

void Invocation::executeBranch(const Instruction *Inst)
{
  moveToBlock(Inst->getOperand(0));
}

void Invocation::executeBranchConditional(const Instruction *Inst)
{
  bool Condition = OP(0, bool);
  moveToBlock(Inst->getOperand(Condition ? 1 : 2));
}

void Invocation::executeCompositeConstruct(const Instruction *Inst)
{
  uint32_t Id = Inst->getOperand(1);

  Object Result = Object(Inst->getResultType());

  // Set constituent values.
  for (uint32_t i = 2; i < Inst->getNumOperands(); i++)
  {
    uint32_t Id = Inst->getOperand(i);
    Result.insert({i - 2}, Objects[Id]);
  }

  Objects[Id] = Result;
}

void Invocation::executeCompositeExtract(const Instruction *Inst)
{
  uint32_t Id = Inst->getOperand(1);
  // TODO: Handle indices of different sizes.
  std::vector<uint32_t> Indices(Inst->getOperands() + 3,
                                Inst->getOperands() + Inst->getNumOperands());
  Objects[Id] = Objects[Inst->getOperand(2)].extract(Indices);
}

void Invocation::executeCompositeInsert(const Instruction *Inst)
{
  uint32_t Id = Inst->getOperand(1);
  Object &Element = Objects[Inst->getOperand(2)];
  // TODO: Handle indices of different sizes.
  std::vector<uint32_t> Indices(Inst->getOperands() + 4,
                                Inst->getOperands() + Inst->getNumOperands());
  assert(Objects[Inst->getOperand(3)].getType()->isComposite());
  Objects[Id] = Objects[Inst->getOperand(3)];
  Objects[Id].insert(Indices, Element);
}

void Invocation::executeControlBarrier(const Instruction *Inst)
{
  // TODO: Handle other execution scopes
  assert(Objects[Inst->getOperand(0)].get<uint32_t>() == SpvScopeWorkgroup);
  AtBarrier = true;
}

void Invocation::executeConvertFToU(const Instruction *Inst)
{
  switch (Inst->getResultType()->getBitWidth())
  {
  case 16:
    executeOpFP<1>(Inst, [](auto A) -> uint16_t { return (uint16_t)A; });
    break;
  case 32:
    executeOpFP<1>(Inst, [](auto A) -> uint32_t { return (uint32_t)A; });
    break;
  case 64:
    executeOpFP<1>(Inst, [](auto A) -> uint64_t { return (uint64_t)A; });
    break;
  default:
    assert(false && "Unhandled integer size for OpConvertFToU");
  }
}

void Invocation::executeConvertSToF(const Instruction *Inst)
{
  switch (Inst->getResultType()->getBitWidth())
  {
  case 32:
    executeOpSInt<1>(Inst, [](auto A) -> float { return (float)A; });
    break;
  case 64:
    executeOpSInt<1>(Inst, [](auto A) -> double { return (double)A; });
    break;
  default:
    assert(false && "Unhandled floating point size for OpConvertUToF");
  }
}

void Invocation::executeConvertUToF(const Instruction *Inst)
{
  switch (Inst->getResultType()->getBitWidth())
  {
  case 32:
    executeOpUInt<1>(Inst, [](auto A) -> float { return (float)A; });
    break;
  case 64:
    executeOpUInt<1>(Inst, [](auto A) -> double { return (double)A; });
    break;
  default:
    assert(false && "Unhandled floating point size for OpConvertUToF");
  }
}

void Invocation::executeCopyMemory(const Instruction *Inst)
{
  const Object &Dst = Objects[Inst->getOperand(0)];
  const Object &Src = Objects[Inst->getOperand(1)];

  const Type *DstType = Dst.getType();
  const Type *SrcType = Src.getType();
  assert(DstType->getElementType() == SrcType->getElementType());

  Memory &DstMem = getMemory(DstType->getStorageClass());
  Memory &SrcMem = getMemory(SrcType->getStorageClass());

  uint64_t DstAddress = Dst.get<uint64_t>();
  uint64_t SrcAddress = Src.get<uint64_t>();
  uint64_t NumBytes = DstType->getElementType()->getSize();
  Memory::copy(DstAddress, DstMem, SrcAddress, SrcMem, NumBytes);
}

void Invocation::executeCopyObject(const Instruction *Inst)
{
  Objects[Inst->getOperand(1)] = Objects[Inst->getOperand(2)];
}

void Invocation::executeDot(const Instruction *Inst)
{
  Object &A = Objects[Inst->getOperand(2)];
  Object &B = Objects[Inst->getOperand(3)];
  switch (Inst->getResultType()->getBitWidth())
  {
  case 32:
  {
    float Result = 0.f;
    for (uint32_t i = 0; i < A.getType()->getElementCount(); i++)
      Result += A.get<float>(i) * B.get<float>(i);
    Objects[Inst->getOperand(1)] = Object(Inst->getResultType(), Result);
    break;
  }
  case 64:
  {
    double Result = 0.0;
    for (uint32_t i = 0; i < A.getType()->getElementCount(); i++)
      Result += A.get<double>(i) * B.get<double>(i);
    Objects[Inst->getOperand(1)] = Object(Inst->getResultType(), Result);
    break;
  }
  default:
    assert(false && "Unhandled floating point size for OpDot");
  }
}

void Invocation::executeExtInst(const Instruction *Inst)
{
  // TODO: Currently assumes extended instruction set is GLSL.std.450
  uint32_t ExtInst = Inst->getOperand(3);
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
  case GLSLstd450FAbs:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return fabs(X); });
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
  case GLSLstd450Sqrt:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return sqrt(X); });
    break;
  case GLSLstd450Tan:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return tan(X); });
    break;
  case GLSLstd450Tanh:
    executeOpFP<1, 4>(Inst, [](auto X) -> decltype(X) { return tanh(X); });
    break;
  default:
    Dev.reportError("Unimplemented GLSL.std.450 extended instruction", true);
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

void Invocation::executeFNegate(const Instruction *Inst)
{
  executeOpFP<1>(Inst, [](auto A) -> decltype(A) { return -A; });
}

void Invocation::executeFOrdEqual(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> bool {
    return A == B && !std::isunordered(A, B);
  });
}

void Invocation::executeFOrdGreaterThan(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> bool {
    return A > B && !std::isunordered(A, B);
  });
}

void Invocation::executeFOrdGreaterThanEqual(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> bool {
    return A >= B && !std::isunordered(A, B);
  });
}

void Invocation::executeFOrdLessThan(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> bool {
    return A < B && !std::isunordered(A, B);
  });
}

void Invocation::executeFOrdLessThanEqual(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> bool {
    return A <= B && !std::isunordered(A, B);
  });
}

void Invocation::executeFOrdNotEqual(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> bool {
    return A != B && !std::isunordered(A, B);
  });
}

void Invocation::executeFRem(const Instruction *Inst)
{
  executeOpFP<2>(Inst,
                 [](auto A, auto B) -> decltype(A) { return fmod(A, B); });
}

void Invocation::executeFSub(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> decltype(A) { return A - B; });
}

void Invocation::executeFunctionCall(const Instruction *Inst)
{
  const Function *Func = CurrentModule->getFunction(Inst->getOperand(2));

  // Copy function parameters.
  assert(Inst->getNumOperands() == Func->getNumParams() + 3);
  for (int i = 3; i < Inst->getNumOperands(); i++)
    Objects[Func->getParamId(i - 3)] = Objects[Inst->getOperand(i)];

  // Create call stack entry.
  StackEntry SE;
  SE.CallInst = Inst;
  SE.CallFunc = CurrentFunction;
  SE.CallBlock = CurrentBlock;
  CallStack.push_back(SE);

  // Move to first block of callee function.
  CurrentFunction = Func;
  moveToBlock(CurrentFunction->getFirstBlockId());
}

void Invocation::executeFUnordEqual(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> bool {
    return A == B || std::isunordered(A, B);
  });
}

void Invocation::executeFUnordGreaterThan(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> bool {
    return A > B || std::isunordered(A, B);
  });
}

void Invocation::executeFUnordGreaterThanEqual(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> bool {
    return A >= B || std::isunordered(A, B);
  });
}

void Invocation::executeFUnordLessThan(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> bool {
    return A < B || std::isunordered(A, B);
  });
}

void Invocation::executeFUnordLessThanEqual(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> bool {
    return A <= B || std::isunordered(A, B);
  });
}

void Invocation::executeFUnordNotEqual(const Instruction *Inst)
{
  executeOpFP<2>(Inst, [](auto A, auto B) -> bool {
    return A != B || std::isunordered(A, B);
  });
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

void Invocation::executeIsInf(const Instruction *Inst)
{
  executeOpFP<1>(Inst, [](auto A) -> bool { return isinf(A); });
}

void Invocation::executeIsNan(const Instruction *Inst)
{
  executeOpFP<1>(Inst, [](auto A) -> bool { return isnan(A); });
}

void Invocation::executeISub(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A - B; });
}

void Invocation::executeLoad(const Instruction *Inst)
{
  uint32_t Id = Inst->getOperand(1);
  const Object &Src = Objects[Inst->getOperand(2)];
  Memory &Mem = getMemory(Src.getType()->getStorageClass());
  Objects[Id] = Object::load(Inst->getResultType(), Mem, Src.get<uint64_t>());
}

void Invocation::executeLogicalAnd(const Instruction *Inst)
{
  executeOp<bool, 2>(Inst, [](bool A, bool B) { return A && B; });
}

void Invocation::executeLogicalEqual(const Instruction *Inst)
{
  executeOp<bool, 2>(Inst, [](bool A, bool B) { return A == B; });
}

void Invocation::executeLogicalNot(const Instruction *Inst)
{
  executeOp<bool, 1>(Inst, [](bool A) { return !A; });
}

void Invocation::executeLogicalNotEqual(const Instruction *Inst)
{
  executeOp<bool, 2>(Inst, [](bool A, bool B) { return A != B; });
}

void Invocation::executeLogicalOr(const Instruction *Inst)
{
  executeOp<bool, 2>(Inst, [](bool A, bool B) { return A || B; });
}

void Invocation::executeNot(const Instruction *Inst)
{
  executeOpUInt<1>(Inst, [](auto A) -> decltype(A) { return ~A; });
}

void Invocation::executePhi(const Instruction *Inst)
{
  uint32_t Id = Inst->getOperand(1);

  assert(PreviousBlock);
  for (int i = 2; i < Inst->getNumOperands(); i += 2)
  {
    assert(i + 1 < Inst->getNumOperands());
    if (Inst->getOperand(i + 1) == PreviousBlock)
    {
      PhiTemps.push_back({Id, Objects[Inst->getOperand(i)]});
      return;
    }
  }
  assert(false && "no matching predecessor block for OpPhi");
}

void Invocation::executePtrAccessChain(const Instruction *Inst)
{
  // Base pointer.
  uint32_t Id = Inst->getOperand(1);
  Object &Base = Objects[Inst->getOperand(2)];

  // Ensure base pointer is valid.
  if (!Base)
  {
    // Check for buffer variable matching base pointer ID.
    for (BufferVariableMap::value_type V : CurrentModule->getBufferVariables())
    {
      if (V.first == Inst->getOperand(2))
      {
        // Report error for missing descriptor set entry.
        std::stringstream Err;
        Err << "Invalid base pointer for descriptor set entry ("
            << V.second.DescriptorSet << "," << V.second.Binding << ")";
        Dev.reportError(Err.str());

        // Set result pointer to null.
        Objects[Id] = Object(Inst->getResultType(), (uint64_t)0);
        return;
      }
    }
    assert(false && "Invalid base pointer for OpPtrAccessChain");
  }

  uint64_t Result = Base.get<uint64_t>();
  const Type *ElemType = Base.getType()->getElementType();

  // Perform initial deference for element index.
  Result += Base.getType()->getElementOffset(OP(3, uint32_t));

  // Loop over indices.
  for (int i = 4; i < Inst->getNumOperands(); i++)
  {
    uint64_t Index;
    const Object &IndexObj = Objects[Inst->getOperand(i)];
    switch (IndexObj.getType()->getSize())
    {
    case 2:
      Index = IndexObj.get<uint16_t>();
      break;
    case 4:
      Index = IndexObj.get<uint32_t>();
      break;
    case 8:
      Index = IndexObj.get<uint64_t>();
      break;
    default:
      Dev.reportError("Unhandled index size", true);
      return;
    }
    Result += ElemType->getElementOffset(Index);
    ElemType = ElemType->getElementType(Index);
  }

  Objects[Id] = Object(Inst->getResultType(), Result);
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

  // Return to calling function.
  CurrentFunction = SE.CallFunc;
  CurrentBlock = SE.CallBlock;
  CurrentInstruction = SE.CallInst->next();
}

void Invocation::executeReturnValue(const Instruction *Inst)
{
  assert(!CallStack.empty());

  StackEntry SE = CallStack.back();
  CallStack.pop_back();

  // Set return value.
  Objects[SE.CallInst->getOperand(1)] = Objects[Inst->getOperand(0)];

  // Release function scope allocations.
  for (uint64_t Address : SE.Allocations)
    PrivateMemory->release(Address);

  // Return to calling function.
  CurrentFunction = SE.CallFunc;
  CurrentBlock = SE.CallBlock;
  CurrentInstruction = SE.CallInst->next();
}

void Invocation::executeSDiv(const Instruction *Inst)
{
  executeOpSInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A / B; });
}

void Invocation::executeSelect(const Instruction *Inst)
{
  bool Condition = OP(2, bool);
  Objects[Inst->getOperand(1)] =
      Objects[Condition ? Inst->getOperand(3) : Inst->getOperand(4)];
}

void Invocation::executeSGreaterThan(const Instruction *Inst)
{
  executeOpSInt<2>(Inst, [](auto A, auto B) -> bool { return A > B; });
}

void Invocation::executeSGreaterThanEqual(const Instruction *Inst)
{
  executeOpSInt<2>(Inst, [](auto A, auto B) -> bool { return A >= B; });
}

void Invocation::executeShiftLeftLogical(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A << B; });
}

void Invocation::executeShiftRightArithmetic(const Instruction *Inst)
{
  executeOpSInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A >> B; });
}

void Invocation::executeShiftRightLogical(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A >> B; });
}

void Invocation::executeSLessThan(const Instruction *Inst)
{
  executeOpSInt<2>(Inst, [](auto A, auto B) -> bool { return A < B; });
}

void Invocation::executeSLessThanEqual(const Instruction *Inst)
{
  executeOpSInt<2>(Inst, [](auto A, auto B) -> bool { return A <= B; });
}

void Invocation::executeSNegate(const Instruction *Inst)
{
  executeOpSInt<1>(Inst, [](auto A) -> decltype(A) { return -A; });
}

void Invocation::executeSRem(const Instruction *Inst)
{
  executeOpSInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A % B; });
}

void Invocation::executeStore(const Instruction *Inst)
{
  uint32_t Id = Inst->getOperand(1);
  const Object &Dest = Objects[Inst->getOperand(0)];
  Memory &Mem = getMemory(Dest.getType()->getStorageClass());
  Objects[Id].store(Mem, Dest.get<uint64_t>());
}

void Invocation::executeSwitch(const Instruction *Inst)
{
  const Object &Selector = Objects[Inst->getOperand(0)];

  // TODO: Handle other selector sizes
  if (Selector.getType()->getBitWidth() != 32)
    Dev.reportError("OpSwitch is only implemented for 32-bit selectors", true);

  for (uint32_t i = 2; i < Inst->getNumOperands(); i += 2)
  {
    if (Selector.get<uint32_t>() == Inst->getOperand(i))
    {
      moveToBlock(Inst->getOperand(i + 1));
      return;
    }
  }
  moveToBlock(Inst->getOperand(1));
}

void Invocation::executeUDiv(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A / B; });
}

void Invocation::executeUGreaterThan(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> bool { return A > B; });
}

void Invocation::executeUGreaterThanEqual(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> bool { return A >= B; });
}

void Invocation::executeULessThan(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> bool { return A < B; });
}

void Invocation::executeULessThanEqual(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> bool { return A <= B; });
}

void Invocation::executeUndef(const Instruction *Inst)
{
  Objects[Inst->getOperand(1)] = Object(Inst->getResultType());
}

void Invocation::executeUnreachable(const Instruction *Inst)
{
  Dev.reportError("OpUnreachable instruction executed", true);
}

void Invocation::executeUMod(const Instruction *Inst)
{
  executeOpUInt<2>(Inst, [](auto A, auto B) -> decltype(A) { return A % B; });
}

void Invocation::executeVariable(const Instruction *Inst)
{
  assert(Inst->getOperand(2) == SpvStorageClassFunction);

  uint32_t Id = Inst->getOperand(1);
  size_t AllocSize = Inst->getResultType()->getElementType()->getSize();
  uint64_t Address = PrivateMemory->allocate(AllocSize);
  Objects[Id] = Object(Inst->getResultType(), Address);

  // Initialize if necessary.
  if (Inst->getNumOperands() > 3)
    Objects[Inst->getOperand(3)].store(*PrivateMemory, Address);

  // Track function scope allocations.
  if (!CallStack.empty())
    CallStack.back().Allocations.push_back(Address);
}

void Invocation::executeVectorShuffle(const Instruction *Inst)
{
  uint32_t Id = Inst->getOperand(1);
  Object Result(Inst->getResultType());

  const Object &Vec1 = Objects[Inst->getOperand(2)];
  const Object &Vec2 = Objects[Inst->getOperand(3)];
  uint32_t Vec1Length = Vec1.getType()->getElementCount();

  for (uint32_t i = 0; i < Inst->getResultType()->getElementCount(); i++)
  {
    uint32_t Idx = Inst->getOperand(4 + i);
    if (Idx < Vec1Length)
      Result.insert({i}, Vec1.extract({Idx}));
    else
      Result.insert({i}, Vec2.extract({Idx - Vec1Length}));
  }

  Objects[Id] = Result;
}

void Invocation::executeVectorTimesScalar(const Instruction *Inst)
{
  switch (Inst->getResultType()->getScalarType()->getBitWidth())
  {
  case 32:
  {
    float Scalar = Objects[Inst->getOperand(3)].get<float>();
    executeOp<float, 1>(Inst, [&](float A) { return A * Scalar; });
    break;
  }
  case 64:
  {
    double Scalar = Objects[Inst->getOperand(3)].get<double>();
    executeOp<double, 1>(Inst, [&](double A) { return A * Scalar; });
    break;
  }
  default:
    assert(false && "Unhandled floating point size for OpDot");
  }
}

Memory &Invocation::getMemory(uint32_t StorageClass)
{
  switch (StorageClass)
  {
  case SpvStorageClassStorageBuffer:
  case SpvStorageClassUniform:
    return Dev.getGlobalMemory();
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

Object Invocation::getObject(uint32_t Id) const
{
  if (Id < Objects.size())
    return Objects[Id];
  else
    return Object();
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
  CurrentInstruction = B->getLabel().next();
  PreviousBlock = CurrentBlock;
  CurrentBlock = Id;
}

void Invocation::step()
{
  assert(getState() == READY);
  assert(CurrentInstruction);

  const Instruction *I = CurrentInstruction;

  if (!PhiTemps.empty() && I->getOpcode() != SpvOpPhi &&
      I->getOpcode() != SpvOpLine)
  {
    for (auto &P : PhiTemps)
      Objects[P.first] = std::move(P.second);
    PhiTemps.clear();
  }

  // Dispatch instruction to handler method.
  uint16_t Opcode = I->getOpcode();
  switch (Opcode)
  {
#define DISPATCH(Op, Func)                                                     \
  case Op:                                                                     \
    execute##Func(I);                                                          \
    break
#define NOP(Op)                                                                \
  case Op:                                                                     \
    break

    DISPATCH(SpvOpAccessChain, AccessChain);
    DISPATCH(SpvOpBitcast, Bitcast);
    DISPATCH(SpvOpBitwiseAnd, BitwiseAnd);
    DISPATCH(SpvOpBitwiseOr, BitwiseOr);
    DISPATCH(SpvOpBitwiseXor, BitwiseXor);
    DISPATCH(SpvOpBranch, Branch);
    DISPATCH(SpvOpBranchConditional, BranchConditional);
    DISPATCH(SpvOpCompositeConstruct, CompositeConstruct);
    DISPATCH(SpvOpCompositeExtract, CompositeExtract);
    DISPATCH(SpvOpCompositeInsert, CompositeInsert);
    DISPATCH(SpvOpControlBarrier, ControlBarrier);
    DISPATCH(SpvOpConvertFToU, ConvertFToU);
    DISPATCH(SpvOpConvertSToF, ConvertSToF);
    DISPATCH(SpvOpConvertUToF, ConvertUToF);
    DISPATCH(SpvOpCopyMemory, CopyMemory);
    DISPATCH(SpvOpCopyObject, CopyObject);
    DISPATCH(SpvOpDot, Dot);
    DISPATCH(SpvOpExtInst, ExtInst);
    DISPATCH(SpvOpFAdd, FAdd);
    DISPATCH(SpvOpFDiv, FDiv);
    DISPATCH(SpvOpFMul, FMul);
    DISPATCH(SpvOpFNegate, FNegate);
    DISPATCH(SpvOpFOrdEqual, FOrdEqual);
    DISPATCH(SpvOpFOrdGreaterThan, FOrdGreaterThan);
    DISPATCH(SpvOpFOrdGreaterThanEqual, FOrdGreaterThanEqual);
    DISPATCH(SpvOpFOrdLessThan, FOrdLessThan);
    DISPATCH(SpvOpFOrdLessThanEqual, FOrdLessThanEqual);
    DISPATCH(SpvOpFOrdNotEqual, FOrdNotEqual);
    DISPATCH(SpvOpFRem, FRem);
    DISPATCH(SpvOpFSub, FSub);
    DISPATCH(SpvOpFunctionCall, FunctionCall);
    DISPATCH(SpvOpFUnordEqual, FUnordEqual);
    DISPATCH(SpvOpFUnordGreaterThan, FUnordGreaterThan);
    DISPATCH(SpvOpFUnordGreaterThanEqual, FUnordGreaterThanEqual);
    DISPATCH(SpvOpFUnordLessThan, FUnordLessThan);
    DISPATCH(SpvOpFUnordLessThanEqual, FUnordLessThanEqual);
    DISPATCH(SpvOpFUnordNotEqual, FUnordNotEqual);
    DISPATCH(SpvOpIAdd, IAdd);
    DISPATCH(SpvOpIEqual, IEqual);
    DISPATCH(SpvOpIMul, IMul);
    DISPATCH(SpvOpINotEqual, INotEqual);
    DISPATCH(SpvOpIsInf, IsInf);
    DISPATCH(SpvOpIsNan, IsNan);
    DISPATCH(SpvOpISub, ISub);
    DISPATCH(SpvOpLoad, Load);
    DISPATCH(SpvOpLogicalEqual, LogicalEqual);
    DISPATCH(SpvOpLogicalNotEqual, LogicalNotEqual);
    DISPATCH(SpvOpLogicalOr, LogicalOr);
    DISPATCH(SpvOpLogicalAnd, LogicalAnd);
    DISPATCH(SpvOpLogicalNot, LogicalNot);
    DISPATCH(SpvOpNot, Not);
    DISPATCH(SpvOpPhi, Phi);
    DISPATCH(SpvOpPtrAccessChain, PtrAccessChain);
    DISPATCH(SpvOpReturn, Return);
    DISPATCH(SpvOpReturnValue, ReturnValue);
    DISPATCH(SpvOpSDiv, SDiv);
    DISPATCH(SpvOpSelect, Select);
    DISPATCH(SpvOpSGreaterThan, SGreaterThan);
    DISPATCH(SpvOpSGreaterThanEqual, SGreaterThanEqual);
    DISPATCH(SpvOpShiftLeftLogical, ShiftLeftLogical);
    DISPATCH(SpvOpShiftRightArithmetic, ShiftRightArithmetic);
    DISPATCH(SpvOpShiftRightLogical, ShiftRightLogical);
    DISPATCH(SpvOpSLessThan, SLessThan);
    DISPATCH(SpvOpSLessThanEqual, SLessThanEqual);
    DISPATCH(SpvOpSNegate, SNegate);
    DISPATCH(SpvOpSRem, SRem);
    DISPATCH(SpvOpStore, Store);
    DISPATCH(SpvOpSwitch, Switch);
    DISPATCH(SpvOpUGreaterThan, UGreaterThan);
    DISPATCH(SpvOpUGreaterThanEqual, UGreaterThanEqual);
    DISPATCH(SpvOpUDiv, UDiv);
    DISPATCH(SpvOpULessThan, ULessThan);
    DISPATCH(SpvOpULessThanEqual, ULessThanEqual);
    DISPATCH(SpvOpUMod, UMod);
    DISPATCH(SpvOpUndef, Undef);
    DISPATCH(SpvOpUnreachable, Unreachable);
    DISPATCH(SpvOpVariable, Variable);
    DISPATCH(SpvOpVectorShuffle, VectorShuffle);
    DISPATCH(SpvOpVectorTimesScalar, VectorTimesScalar);

    NOP(SpvOpNop);
    NOP(SpvOpLine);
    NOP(SpvOpLoopMerge);
    NOP(SpvOpNoLine);
    NOP(SpvOpSelectionMerge);

#undef DISPATCH
#undef NOP

  default:
    Dev.reportError("Unimplemented instruction", true);
  }

  // Move program counter to next instruction, unless a terminator instruction
  // was executed.
  if (I == CurrentInstruction)
    CurrentInstruction = CurrentInstruction->next();

  Dev.reportInstructionExecuted(this, I);

  if (getState() == FINISHED)
    Dev.reportInvocationComplete(this);
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
  uint32_t Id = Inst->getOperand(1);
  Object Result(Inst->getResultType());
  std::array<OpTy, N> Operands;

  // Loop over each vector component.
  for (uint32_t i = 0; i < Inst->getResultType()->getElementCount(); i++)
  {
    // Gather operands.
    for (unsigned j = 0; j < N; j++)
      Operands[j] = Objects[Inst->getOperand(Offset + j)].get<OpTy>(i);

    // Apply lambda and set result.
    Result.set(apply(Operands, Op), i);
  }

  Objects[Id] = Result;
}

template <unsigned N, unsigned Offset, typename F>
void Invocation::executeOpSInt(const Instruction *Inst, const F &&Op)
{
  const Type *OpType = Objects[Inst->getOperand(Offset)].getType();
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
  const Type *OpType = Objects[Inst->getOperand(Offset)].getType();
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
  const Type *OpType = Objects[Inst->getOperand(Offset)].getType();
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
