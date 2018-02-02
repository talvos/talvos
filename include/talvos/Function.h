// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_FUNCTION_H
#define TALVOS_FUNCTION_H

#include <map>
#include <memory>

namespace talvos
{

class Block;
class Type;

/// This class represents a function in a SPIR-V Module.
class Function
{
public:
  Function(uint32_t Id, const Type *FunctionType);

  /// Add a block to this function.
  void addBlock(std::unique_ptr<Block> B);

  /// Returns the block with ID \p Id.
  const Block *getBlock(uint32_t Id) const { return Blocks.at(Id).get(); }

  /// Returns the first block in this function.
  const Block *getEntryBlock() const { return Blocks.at(EntryBlockId).get(); }

  uint32_t getEntryBlockId() const { return EntryBlockId; }
  uint32_t getId() { return Id; }
  void setEntryBlock(uint32_t Id) { EntryBlockId = Id; }

private:
  uint32_t Id;
  const Type *FunctionType;
  std::map<uint32_t, std::unique_ptr<Block>> Blocks;
  uint32_t EntryBlockId;
};

} // namespace talvos

#endif
