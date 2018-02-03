// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "talvos/Instruction.h"

namespace talvos
{

void Instruction::insertAfter(Instruction *I)
{
  this->Next = std::move(I->Next);
  I->Next = std::unique_ptr<Instruction>(this);
}

} // namespace talvos
