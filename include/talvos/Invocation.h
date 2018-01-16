// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_INVOCATION_H
#define TALVOS_INVOCATION_H

namespace talvos
{

struct Function;
struct Instruction;

class Invocation
{
public:
  enum State { READY, FINISHED };

public:
  Invocation(const Function *F);
  State getState() const;
  void step();

  // Instruction handlers.
  void executeAccessChain(const Instruction *Inst);
  void executeLoad(const Instruction *Inst);
  void executeStore(const Instruction *Inst);

private:
  const Instruction *CurrentInstruction;
};

} // namespace talvos

#endif
