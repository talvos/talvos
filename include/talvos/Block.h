// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Block.h
/// This file declares the Block class.

#ifndef TALVOS_BLOCK_H
#define TALVOS_BLOCK_H

#include <cstdint>
#include <memory>

namespace talvos
{

class Instruction;

/// A block of instructions ending with a termination instruction.
class Block
{
public:
  /// Create a new block with an ID.
  Block(uint32_t Id);

  ~Block();

  // Do not allow Block objects to be copied.
  ///\{
  Block(const Block &) = delete;
  Block &operator=(const Block &) = delete;
  ///\}

  /// Returns the ID of this block.
  uint32_t getId() const { return Id; }

  /// Returns the label instruction for this block.
  Instruction &getLabel() const { return *Label.get(); }

private:
  uint32_t Id; ///< The unique ID of the block.

  std::unique_ptr<Instruction> Label; ///< The label instruction.
};

} // namespace talvos

#endif
