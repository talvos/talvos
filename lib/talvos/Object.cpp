// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <vector>

#include "talvos/Object.h"
#include "talvos/Memory.h"

namespace talvos
{

Object Object::extract(const std::vector<uint32_t> &Indices) const
{
  assert(Data);

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

void Object::insert(const std::vector<uint32_t> &Indices, const Object &Element)
{
  assert(Data);

  // Loop over indices to compute byte offset.
  uint32_t Offset = 0;
  const Type *Ty = this->Ty;
  for (size_t i = 0; i < Indices.size(); i++)
  {
    assert(Ty->isComposite());
    Offset += Ty->getElementOffset(Indices[i]);
    Ty = Ty->getElementType(Indices[i]);
  }

  // Copy element data.
  assert(Ty == Element.Ty);
  memcpy(Data + Offset, Element.Data, Ty->getSize());
}

Object Object::load(const Type *Ty, const Memory &Mem, size_t Address)
{
  Object Result;
  Result.Ty = Ty;
  Result.Data = new uint8_t[Ty->getSize()];
  Mem.load(Result.Data, Address, Ty->getSize());
  return Result;
}

void Object::store(Memory &Mem, size_t Address) const
{
  assert(Data);
  Mem.store(Address, Ty->getSize(), Data);
}

} // namespace talvos
