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

struct Function;
struct Instruction;
class Device;
class Memory;
class Module;

class Invocation
{
public:
  enum State { READY, FINISHED };

public:
  Invocation(Device *D, const Module *M, const Function *F,
             const std::vector<std::pair<uint32_t, size_t>> &Variables);
  ~Invocation();
  State getState() const;
  void step();

  // Instruction handlers.
  void executeAccessChain(const Instruction *Inst);
  void executeLoad(const Instruction *Inst);
  void executeStore(const Instruction *Inst);

private:
  const Instruction *CurrentInstruction;
  std::vector<Object> Objects;
  Device *Dev;
};

} // namespace talvos

#endif
