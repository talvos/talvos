// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_INSTRUCTION_H
#define TALVOS_INSTRUCTION_H

#include <cstdint>
#include <memory>

namespace talvos
{

class Type;

/// This class represents a SPIR-V instruction.
///
/// An instance of this class has an opcode, a set of operands, and possibly a
/// return type. It is also a node in a linked list of instructions, forming a
/// sequence that represents the instructions within a block. After creation, an
/// Instruction should be inserted into a instruction sequence or the beginning
/// of a block.
class Instruction
{
public:
  /// Create a new instruction.
  Instruction() : Next(nullptr) {}

  /// Destroy this instruction.
  /// If the instruction has been inserted into a sequence, this will also
  /// destroy any instructions that follow it.
  ~Instruction() { delete[] Operands; }

  Instruction(const Instruction &) = delete;
  const Instruction &operator=(const Instruction &) = delete;

  /// Insert this instruction into a sequence, immediately following \p I.
  /// This transfers ownership of this instruction to the containing block.
  void insertAfter(Instruction *I);

  /// Get the next instruction in the containing block.
  /// /returns the next instruction, or nullptr if this instruction is a
  /// terminator.
  const Instruction *next() const { return Next.get(); }

  /// Print a human-readable form of this instruction to \p O.
  void print(std::ostream &O) const;

  /// Return the string representation of an instruction opcode.
  static const char *opToStr(uint16_t Opcode);

  // TODO: Make these private with getters once operands sorted out properly
  const Type *ResultType;
  uint16_t Opcode;
  uint16_t NumOperands;
  // TODO: Currently assumes all operands are 32-bit IDs
  uint32_t *Operands;

private:
  std::unique_ptr<Instruction> Next; ///< The next instruction in the block.
};

} // namespace talvos

#endif
