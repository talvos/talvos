// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <vector>

#include "talvos/Object.h"
#include "talvos/Memory.h"

namespace talvos
{

Object Object::createComposite(const Type *Ty,
                               const std::vector<Object> &Elements)
{
  Object Result;
  Result.Ty = Ty;
  Result.Data = new uint8_t[Ty->getSize()];
  for (size_t i = 0; i < Elements.size(); i++)
  {
    assert(Ty->getElementType(i) == Elements[i].getType());
    memcpy(Result.Data + Ty->getElementOffset(i), Elements[i].Data,
           Elements[i].getType()->getSize());
  }
  return Result;
}

Object Object::extract(const std::vector<uint32_t> &Indices) const
{
  // Loop over indices to compute byte offset and result type.
  uint32_t Offset = 0;
  const Type *Ty = this->Ty;
  for (size_t i = 0; i < Indices.size(); i++)
  {
    assert(Ty->isComposite());
    Offset += Ty->getElementOffset(Indices[i]);
    Ty = Ty->getElementType(Indices[i]);
  }

  // Create result object and copy data over.
  Object Result;
  Result.Ty = Ty;
  Result.Data = new uint8_t[Ty->getSize()];
  memcpy(Result.Data, Data + Offset, Ty->getSize());
  return Result;
}

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
