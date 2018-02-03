// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "talvos/Block.h"
#include "talvos/Instruction.h"

namespace talvos
{

Block::Block(uint32_t Id)
{
  this->Id = Id;
  FirstInstruction = nullptr;
}

Block::~Block() {}

const Instruction *Block::getFirstInstruction() const
{
  return FirstInstruction.get();
}

void Block::insertAtStart(Instruction *I)
{
  if (FirstInstruction)
    FirstInstruction.release()->insertAfter(I);
  FirstInstruction = std::unique_ptr<Instruction>(I);
}

} // namespace talvos
