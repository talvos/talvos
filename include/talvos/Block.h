// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_BLOCK_H
#define TALVOS_BLOCK_H

#include <cstdint>

namespace talvos
{

struct Instruction;

/// A block of instruction ending with a termination instruction.
class Block
{
public:
  /// Create a new block with an ID.
  Block(uint32_t Id) : Id(Id), FirstInstruction(nullptr) {}

  ~Block();
  Block(const Block &) = delete;
  Block &operator=(const Block &) = delete;

  /// Returns the ID of this block.
  uint32_t getId() const { return Id; }

private:
  uint32_t Id; ///< The unique ID of the block.

public:
  // TODO: Make this private once we have proper instruction insertion routines.
  Instruction *FirstInstruction; ///< The first instruction in the block.
};

} // namespace talvos

#endif
