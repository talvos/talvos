// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Variable.cpp
/// This file defines the Variable class.

#include <cassert>
#include <spirv/unified1/spirv.h>

#include "talvos/Type.h"
#include "talvos/Variable.h"

namespace talvos
{

Variable::Variable(uint32_t Id, const Type *Ty, uint32_t Initializer)
    : Id(Id), Ty(Ty), Initializer(Initializer)
{}

void Variable::addDecoration(uint32_t Decoration, uint32_t Data)
{
  Decorations[Decoration] = Data;
}

uint32_t Variable::getDecoration(uint32_t Decoration) const
{
  assert(Decorations.count(Decoration));
  return Decorations.at(Decoration);
}

bool Variable::hasDecoration(uint32_t Decoration) const
{
  return Decorations.count(Decoration);
}

bool Variable::isBufferVariable() const
{
  if (Ty->getStorageClass() == SpvStorageClassStorageBuffer ||
      Ty->getStorageClass() == SpvStorageClassUniform ||
      Ty->getStorageClass() == SpvStorageClassUniformConstant)
    return true;
  else
    return false;
}

} // namespace talvos
