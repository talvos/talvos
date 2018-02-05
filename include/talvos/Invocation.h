// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_INVOCATION_H
#define TALVOS_INVOCATION_H

#include <vector>

#include "talvos/Object.h"

namespace talvos
{

class Device;
class Function;
class Instruction;
class Memory;
class Module;

/// This class represents a single execution of a SPIR-V entry point.
///
/// Instances of this class have a current instruction, block, and function, as
/// well as their own Memory object used for private memory allocations. They
/// also have a set of Objects used to hold instruction results.
class Invocation
{
public:
  /// Used to indicate whether an invocation is ready to execute, or complete.
  enum State { READY, FINISHED };

public:
  /// Create an invocation of the entry point \p F on a particular device.
  /// Global variables with their resolved pointer values are listed in
  /// \p Variables.
  Invocation(Device *D, const Module *M, const Function *F, uint32_t GroupIdX,
             uint32_t GroupIdY, uint32_t GroupIdZ,
             const std::vector<std::pair<uint32_t, Object>> &Variables);

  /// Destroy this invocation.
  ~Invocation();

  /// Returns the state of this invocation.
  State getState() const;

  /// Step this invocation by executing the next instruction.
  void step();

  /// Instruction handlers.
  ///@{
  void executeAccessChain(const Instruction *Inst);
  void executeBranch(const Instruction *Inst);
  void executeBranchConditional(const Instruction *Inst);
  void executeCompositeExtract(const Instruction *Inst);
  void executeIAdd(const Instruction *Inst);
  void executeIEqual(const Instruction *Inst);
  void executeIMul(const Instruction *Inst);
  void executeLoad(const Instruction *Inst);
  void executePhi(const Instruction *Inst);
  void executeReturn(const Instruction *Inst);
  void executeStore(const Instruction *Inst);
  ///@}

private:
  const Function *CurrentFunction;       ///< The current function.
  const Instruction *CurrentInstruction; ///< The current instruction.
  uint32_t CurrentBlock;                 ///< The current block.
  uint32_t PreviousBlock;                ///< The previous block (for OpPhi).

  std::vector<Object> Objects; ///< Set of result objects.

  Device *Dev;           ///< The device this invocation is executing on.
  uint32_t GlobalId[3];  ///< The GlobalInvocationID.
  Memory *PrivateMemory; ///< The private memory instance.

  /// Helper function to execute binary instructions.
  /// \p F is a lambda that takes two operand values and returns the result.
  template <typename F>
  void executeBinaryOp(const Instruction *Inst, const F &&Op);

  /// Returns the memory instance associated with \p StorageClass.
  Memory &getMemory(uint32_t StorageClass);

  /// Move this invocation to the block with ID \p Id.
  void moveToBlock(uint32_t Id);
};

} // namespace talvos

#endif
