// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Instruction.h
/// This file declares the Instruction class.

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
  Instruction(uint16_t Opcode, uint16_t NumOperands, const uint32_t *Operands,
              const Type *ResultType);

  /// Destroy this instruction.
  /// If the instruction has been inserted into a sequence, this will also
  /// destroy any instructions that follow it.
  ~Instruction() { delete[] Operands; }

  // Do not allow Instruction objects to be copied.
  ///\{
  Instruction(const Instruction &) = delete;
  const Instruction &operator=(const Instruction &) = delete;
  ///\}

  /// Returns the number of operands this instruction has.
  uint16_t getNumOperands() const { return NumOperands; }

  /// Returns the opcode.
  uint16_t getOpcode() const { return Opcode; }

  /// Returns the operand at index \p i;
  uint32_t getOperand(unsigned i) const { return Operands[i]; }

  /// Returns the operands.
  const uint32_t *getOperands() const { return Operands; }

  /// Returns the result type of this instruction, or \p nullptr if it does not
  /// produce a result.
  const Type *getResultType() const { return ResultType; }

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

private:
  const Type *ResultType; ///< The type of the instruction result.
  uint16_t Opcode;        ///< The instruction opcode.
  uint16_t NumOperands;   ///< The number of operands in this instruction.
  uint32_t *Operands;     ///< The operand values.

  std::unique_ptr<Instruction> Next; ///< The next instruction in the block.
};

} // namespace talvos

#endif
