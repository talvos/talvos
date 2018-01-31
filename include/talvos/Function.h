// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_FUNCTION_H
#define TALVOS_FUNCTION_H

#include <map>

namespace talvos
{

struct Instruction;
class Type;

// TODO: Make Block a class as well?
struct Block
{
  uint32_t Id;
  const Instruction *FirstInstruction;
};

class Function
{
public:
  Function(uint32_t Id, const Type *FunctionType);
  void addBlock(Block *B);
  const Block *getBlock(uint32_t Id) const { return Blocks.at(Id); }
  const Block *getEntryBlock() const { return Blocks.at(EntryBlockId); }
  uint32_t getEntryBlockId() const { return EntryBlockId; }
  uint32_t getId() { return Id; }
  void setEntryBlock(uint32_t Id) { EntryBlockId = Id; }

private:
  uint32_t Id;
  const Type *FunctionType;
  std::map<uint32_t, Block *> Blocks;
  uint32_t EntryBlockId;
};

} // namespace talvos

#endif
