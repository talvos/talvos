// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Block.cpp
/// This file defines the Block class.

#include "talvos/Block.h"
#include "talvos/Instruction.h"

#include <spirv/unified1/spirv.h>

namespace talvos
{

Block::Block(uint32_t Id)
{
  this->Id = Id;
  Label = std::make_unique<Instruction>(SpvOpLabel, 1, &Id, nullptr);
}

Block::~Block() {}

} // namespace talvos
