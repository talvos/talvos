// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "talvos/Object.h"
#include "talvos/Memory.h"

namespace talvos
{

Object Object::load(const Type *Ty, Memory *Mem, size_t Address)
{
  Object Obj;
  Obj.Ty = Ty;
  Obj.Data = new uint8_t[Ty->getSize()];
  Mem->load(Obj.Data, Address, Ty->getSize());
  return Obj;
}

void Object::store(Memory *Mem, size_t Address) const
{
  Mem->store(Address, Ty->getSize(), Data);
}

} // namespace talvos
