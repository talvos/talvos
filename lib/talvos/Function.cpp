// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <cassert>

#include "talvos/Function.h"
#include "talvos/Type.h"

namespace talvos
{

Function::Function(uint32_t Id, const Type *FuncType)
{
  this->Id = Id;
  this->FunctionType = FuncType;
}

void Function::addBlock(Block *B)
{
  assert(Blocks.count(B->Id) == 0);
  Blocks[B->Id] = B;
}

} // namespace talvos
