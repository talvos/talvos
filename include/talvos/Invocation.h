// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Invocation.h
/// This file declares the Invocation class.

#ifndef TALVOS_INVOCATION_H
#define TALVOS_INVOCATION_H

#include <vector>

#include "talvos/Dim3.h"
#include "talvos/Object.h"

namespace talvos
{

class Device;
class Function;
class Instruction;
class Memory;
class Module;
class PipelineStage;
class Workgroup;

/// This class represents a single execution of a SPIR-V entry point.
///
/// Instances of this class have a current instruction, block, and function, as
/// well as their own Memory object used for private memory allocations. They
/// also have a set of Objects used to hold instruction results.
class Invocation
{
public:
  /// Used to indicate whether an invocation is ready to execute, waiting at a
  /// barrier, or complete.
  enum State
  {
    READY,
    BARRIER,
    FINISHED
  };

public:
  /// Create a standalone invocation for a device, with an initial set of
  /// result objects.
  Invocation(Device &Dev, const std::vector<Object> &InitialObjects);

  /// Create an invocation for \p Stage on \p Dev. Initial result object values
  /// are provided in \p InitialObjects, and \p PipelineMemory provides storage
  /// for input and output memory accesses.
  Invocation(Device &Dev, const PipelineStage &Stage,
             const std::vector<Object> &InitialObjects,
             std::shared_ptr<Memory> PipelineMemory, Workgroup *Group,
             Dim3 GlobalId);

  /// Destroy this invocation.
  ~Invocation();

  // Do not allow Invocation objects to be copied.
  ///\{
  Invocation(const Invocation &) = delete;
  Invocation &operator=(const Invocation &) = delete;
  ///\}

  /// Clear the barrier state, allowing the invocation to continue.
  void clearBarrier() { AtBarrier = false; }

  /// Execute \p Inst in this invocation.
  void execute(const Instruction *Inst);

  /// Returns the instruction that this invocation is executing.
  const Instruction *getCurrentInstruction() const
  {
    return CurrentInstruction;
  }

  /// Returns the global invocation ID.
  Dim3 getGlobalId() const { return GlobalId; }

  /// Returns the object with the specified ID.
  /// Returns a null object if no object with this ID has been defined.
  Object getObject(uint32_t Id) const;

  /// Returns the state of this invocation.
  State getState() const;

  /// Step this invocation by executing the next instruction.
  void step();

  /// \name Instruction handlers.
  ///@{
  void executeAccessChain(const Instruction *Inst);
  void executeAll(const Instruction *Inst);
  void executeAny(const Instruction *Inst);
  void executeBitcast(const Instruction *Inst);
  void executeBitwiseAnd(const Instruction *Inst);
  void executeBitwiseOr(const Instruction *Inst);
  void executeBitwiseXor(const Instruction *Inst);
  void executeBranch(const Instruction *Inst);
  void executeBranchConditional(const Instruction *Inst);
  void executeCompositeConstruct(const Instruction *Inst);
  void executeCompositeExtract(const Instruction *Inst);
  void executeCompositeInsert(const Instruction *Inst);
  void executeControlBarrier(const Instruction *Inst);
  void executeConvertFToS(const Instruction *Inst);
  void executeConvertFToU(const Instruction *Inst);
  void executeConvertSToF(const Instruction *Inst);
  void executeConvertUToF(const Instruction *Inst);
  void executeCopyMemory(const Instruction *Inst);
  void executeCopyObject(const Instruction *Inst);
  void executeDot(const Instruction *Inst);
  void executeExtInst(const Instruction *Inst);
  void executeFAdd(const Instruction *Inst);
  void executeFConvert(const Instruction *Inst);
  void executeFDiv(const Instruction *Inst);
  void executeFMod(const Instruction *Inst);
  void executeFMul(const Instruction *Inst);
  void executeFNegate(const Instruction *Inst);
  void executeFOrdEqual(const Instruction *Inst);
  void executeFOrdGreaterThan(const Instruction *Inst);
  void executeFOrdGreaterThanEqual(const Instruction *Inst);
  void executeFOrdLessThan(const Instruction *Inst);
  void executeFOrdLessThanEqual(const Instruction *Inst);
  void executeFOrdNotEqual(const Instruction *Inst);
  void executeFRem(const Instruction *Inst);
  void executeFSub(const Instruction *Inst);
  void executeFunctionCall(const Instruction *Inst);
  void executeFUnordEqual(const Instruction *Inst);
  void executeFUnordGreaterThan(const Instruction *Inst);
  void executeFUnordGreaterThanEqual(const Instruction *Inst);
  void executeFUnordLessThan(const Instruction *Inst);
  void executeFUnordLessThanEqual(const Instruction *Inst);
  void executeFUnordNotEqual(const Instruction *Inst);
  void executeIAdd(const Instruction *Inst);
  void executeIEqual(const Instruction *Inst);
  void executeIMul(const Instruction *Inst);
  void executeINotEqual(const Instruction *Inst);
  void executeIsInf(const Instruction *Inst);
  void executeIsNan(const Instruction *Inst);
  void executeISub(const Instruction *Inst);
  void executeLoad(const Instruction *Inst);
  void executeLogicalAnd(const Instruction *Inst);
  void executeLogicalEqual(const Instruction *Inst);
  void executeLogicalNot(const Instruction *Inst);
  void executeLogicalNotEqual(const Instruction *Inst);
  void executeLogicalOr(const Instruction *Inst);
  void executeMatrixTimesScalar(const Instruction *Inst);
  void executeNot(const Instruction *Inst);
  void executePhi(const Instruction *Inst);
  void executeReturn(const Instruction *Inst);
  void executeReturnValue(const Instruction *Inst);
  void executeSConvert(const Instruction *Inst);
  void executeSDiv(const Instruction *Inst);
  void executeSelect(const Instruction *Inst);
  void executeSGreaterThan(const Instruction *Inst);
  void executeSGreaterThanEqual(const Instruction *Inst);
  void executeShiftLeftLogical(const Instruction *Inst);
  void executeShiftRightArithmetic(const Instruction *Inst);
  void executeShiftRightLogical(const Instruction *Inst);
  void executeSLessThan(const Instruction *Inst);
  void executeSLessThanEqual(const Instruction *Inst);
  void executeSMod(const Instruction *Inst);
  void executeSNegate(const Instruction *Inst);
  void executeSRem(const Instruction *Inst);
  void executeStore(const Instruction *Inst);
  void executeSwitch(const Instruction *Inst);
  void executeUConvert(const Instruction *Inst);
  void executeUDiv(const Instruction *Inst);
  void executeUGreaterThan(const Instruction *Inst);
  void executeUGreaterThanEqual(const Instruction *Inst);
  void executeULessThan(const Instruction *Inst);
  void executeULessThanEqual(const Instruction *Inst);
  void executeUMod(const Instruction *Inst);
  void executeUndef(const Instruction *Inst);
  void executeUnreachable(const Instruction *Inst);
  void executeVariable(const Instruction *Inst);
  void executeVectorExtractDynamic(const Instruction *Inst);
  void executeVectorInsertDynamic(const Instruction *Inst);
  void executeVectorShuffle(const Instruction *Inst);
  void executeVectorTimesScalar(const Instruction *Inst);
  ///@}

private:
  /// The current module.
  std::shared_ptr<const Module> CurrentModule;

  const Function *CurrentFunction;       ///< The current function.
  const Instruction *CurrentInstruction; ///< The current instruction.
  uint32_t CurrentBlock;                 ///< The current block.
  uint32_t PreviousBlock;                ///< The previous block (for OpPhi).
  bool AtBarrier;                        ///< True when at a barrier.

  /// A data structure holding information for a function call.
  struct StackEntry
  {
    // The instruction, block, and function to return to.
    const Instruction *CallInst; ///< The calling instruction.
    const Function *CallFunc;    ///< The function containing \p CallInst.
    uint32_t CallBlock;          ///< The block containing \p CallInst.

    /// Function scope allocations within this stack frame.
    std::vector<uint64_t> Allocations;
  };

  std::vector<StackEntry> CallStack; ///< The function call stack.

  std::vector<Object> Objects; ///< Set of result objects.

  Device &Dev;           ///< The device this invocation is executing on.
  Workgroup *Group;      ///< The workgroup this invocation belongs to.
  Dim3 GlobalId;         ///< The GlobalInvocationID.
  Memory *PrivateMemory; ///< The private memory instance.

  /// Memory used for input and output storage classes.
  std::shared_ptr<Memory> PipelineMemory;

  /// Temporary OpPhi results to be applied when we reach first non-OpPhi.
  std::vector<std::pair<uint32_t, Object>> PhiTemps;

  /// Helper functions to execute simple instructions that can either operate
  /// on scalars or component-wise for vectors.
  /// \p OpTy is the C++ scalar type of each operand.
  /// \p N is the number of operands.
  /// \p Offset is the operand offset of the first value.
  /// \p Op is a lambda that takes \p N operand values and returns a result.
  ///@{
  template <typename OpTy, unsigned N, unsigned Offset = 2, typename F>
  void executeOp(const Instruction *Inst, const F &Op);
  template <unsigned N, unsigned Offset = 2, typename F>
  void executeOpFP(const Instruction *Inst, const F &&Op);
  template <unsigned N, unsigned Offset = 2, typename F>
  void executeOpSInt(const Instruction *Inst, const F &&Op);
  template <unsigned N, unsigned Offset = 2, typename F>
  void executeOpUInt(const Instruction *Inst, const F &&Op);
  ///@}

  /// Returns the memory instance associated with \p StorageClass.
  Memory &getMemory(uint32_t StorageClass);

  /// Move this invocation to the block with ID \p Id.
  void moveToBlock(uint32_t Id);
};

} // namespace talvos

#endif
