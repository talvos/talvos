// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_INVOCATION_H
#define TALVOS_INVOCATION_H

#include <vector>

#include "talvos/Dim3.h"
#include "talvos/Object.h"

namespace talvos
{

class Device;
class DispatchCommand;
class Function;
class Instruction;
class Memory;
class Module;
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
  /// Create an invocation for \p Dispatch, with specific group and local IDs.
  /// Global variables with their resolved pointer values are listed in
  /// \p Variables.
  Invocation(const DispatchCommand *Dispatch, Workgroup *Group, Dim3 LocalId);

  /// Destroy this invocation.
  ~Invocation();

  // Do not allow Invocations to be copied.
  Invocation(const Invocation &) = delete;
  Invocation &operator=(const Invocation &) = delete;

  /// Clear the barrier state, allowing the invocation to continue.
  void clearBarrier() { AtBarrier = false; }

  /// Returns the state of this invocation.
  State getState() const;

  /// Step this invocation by executing the next instruction.
  void step();

  /// Instruction handlers.
  ///@{
  void executeAccessChain(const Instruction *Inst);
  void executeBitwiseAnd(const Instruction *Inst);
  void executeBitwiseOr(const Instruction *Inst);
  void executeBranch(const Instruction *Inst);
  void executeBranchConditional(const Instruction *Inst);
  void executeCompositeExtract(const Instruction *Inst);
  void executeCompositeInsert(const Instruction *Inst);
  void executeControlBarrier(const Instruction *Inst);
  void executeExtInst(const Instruction *Inst);
  void executeFAdd(const Instruction *Inst);
  void executeFDiv(const Instruction *Inst);
  void executeFMul(const Instruction *Inst);
  void executeFSub(const Instruction *Inst);
  void executeFunctionCall(const Instruction *Inst);
  void executeIAdd(const Instruction *Inst);
  void executeIEqual(const Instruction *Inst);
  void executeIMul(const Instruction *Inst);
  void executeLoad(const Instruction *Inst);
  void executeLogicalNot(const Instruction *Inst);
  void executePhi(const Instruction *Inst);
  void executePtrAccessChain(const Instruction *Inst);
  void executeReturn(const Instruction *Inst);
  void executeReturnValue(const Instruction *Inst);
  void executeSGreaterThan(const Instruction *Inst);
  void executeShiftRightLogical(const Instruction *Inst);
  void executeSLessThan(const Instruction *Inst);
  void executeStore(const Instruction *Inst);
  void executeULessThan(const Instruction *Inst);
  void executeUndef(const Instruction *Inst);
  void executeVariable(const Instruction *Inst);
  ///@}

private:
  const Module *CurrentModule;           ///< The current module.
  const Function *CurrentFunction;       ///< The current function.
  const Instruction *CurrentInstruction; ///< The current instruction.
  uint32_t CurrentBlock;                 ///< The current block.
  uint32_t PreviousBlock;                ///< The previous block (for OpPhi).
  bool AtBarrier;                        ///< True when at a barrier.

  /// A data structure holding information for a function call.
  struct StackEntry
  {
    // The instruction, block, and function to return to.
    const Instruction *RetInst;
    const Function *RetFunc;
    uint32_t RetBlock;

    /// Function scope allocations within this stack frame.
    std::vector<uint64_t> Allocations;
  };

  std::vector<StackEntry> CallStack; ///< The function call stack.

  std::vector<Object> Objects; ///< Set of result objects.

  Device *Dev;           ///< The device this invocation is executing on.
  Workgroup *Group;      ///< The workgroup this invocation belongs to.
  Dim3 GroupId;          ///< The WorkgroupId.
  Dim3 LocalId;          ///< The LocalInvocationID.
  Dim3 GlobalId;         ///< The GlobalInvocationID.
  Memory *PrivateMemory; ///< The private memory instance.

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
