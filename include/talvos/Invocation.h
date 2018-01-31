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

struct Instruction;
class Function;
class Device;
class Memory;
class Module;

class Invocation
{
public:
  enum State { READY, FINISHED };

public:
  Invocation(Device *D, const Module *M, const Function *F, uint32_t GroupIdX,
             uint32_t GroupIdY, uint32_t GroupIdZ,
             const std::vector<std::pair<uint32_t, Object>> &Variables);
  ~Invocation();
  State getState() const;
  void step();

  // Instruction handlers.
  void executeAccessChain(const Instruction *Inst);
  void executeBranch(const Instruction *Inst);
  void executeBranchConditional(const Instruction *Inst);
  void executeCompositeExtract(const Instruction *Inst);
  void executeIAdd(const Instruction *Inst);
  void executeIEqual(const Instruction *Inst);
  void executeLoad(const Instruction *Inst);
  void executePhi(const Instruction *Inst);
  void executeReturn(const Instruction *Inst);
  void executeStore(const Instruction *Inst);

private:
  const Function *CurrentFunction;
  const Instruction *CurrentInstruction;
  uint32_t CurrentBlock;
  uint32_t PreviousBlock;
  std::vector<Object> Objects;
  Device *Dev;

  uint32_t GlobalId[3];
  Memory *PrivateMemory;

  template <typename F>
  void executeBinaryOp(const Instruction *Inst, const F &&Op);
  Memory *getMemory(uint32_t StorageClass);
  void moveToBlock(uint32_t Id);
};

} // namespace talvos

#endif
