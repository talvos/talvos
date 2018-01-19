// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "talvos/Object.h"
#include "talvos/Memory.h"

namespace talvos
{

Object Object::load(Memory *Mem, size_t Address)
{
  // TODO: Use actual type size
  Object Obj;
  Obj.Data = new uint8_t[4];
  Mem->load(Obj.Data, Address, 4);
  return Obj;
}

void Object::store(Memory *Mem, size_t Address) const
{
  // TODO: Use actual type size
  Mem->store(Address, 4, Data);
}

} // namespace talvos
