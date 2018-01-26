// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_INSTRUCTION_H
#define TALVOS_INSTRUCTION_H

#include <cstdint>

namespace talvos
{

class Type;

// TODO: Class instead? Subclasses for specific instructions?
struct Instruction
{
  const Type *ResultType;
  uint16_t Opcode;
  uint16_t NumOperands;
  // TODO: Currently assumes all operands are 32-bit IDs
  uint32_t *Operands;
  const Instruction *Next;
};

} // namespace talvos

#endif
