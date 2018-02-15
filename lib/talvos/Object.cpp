// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <cassert>
#include <cstring>
#include <vector>

#include "talvos/Object.h"
#include "talvos/Memory.h"

namespace talvos
{

Object::Object(const Type *Ty)
{
  assert(Ty);
  this->Ty = Ty;
  this->Data = new uint8_t[Ty->getSize()];
}

template <typename T> Object::Object(const Type *Ty, T Value) : Object(Ty)
{
  assert(Ty->isScalar());
  assert(sizeof(T) == Ty->getSize());
  *((T *)Data) = Value;
}

Object::~Object() { delete[] Data; }

Object::Object(const Object &Src)
{
  Data = nullptr;
  if (Src)
  {
    Ty = Src.Ty;
    Data = new uint8_t[Ty->getSize()];
    memcpy(Data, Src.Data, Ty->getSize());
  }
}

Object &Object::operator=(const Object &Src)
{
  if (this != &Src)
  {
    Object Tmp(Src);
    std::swap(Data, Tmp.Data);
    std::swap(Ty, Tmp.Ty);
  }
  return *this;
}

Object::Object(Object &&Src) noexcept
{
  Ty = Src.Ty;
  Data = Src.Data;
  Src.Data = nullptr;
}

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

template <typename T> T Object::get(uint32_t Element) const
{
  assert(Data);
  assert(Ty->isScalar() || Ty->isVector());
  assert(Ty->isScalar() ? (sizeof(T) == Ty->getSize() && Element == 0)
                        : sizeof(T) == Ty->getElementType()->getSize());
  return ((T *)Data)[Element];
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

Object Object::load(const Type *Ty, const Memory &Mem, uint64_t Address)
{
  Object Result;
  Result.Ty = Ty;
  Result.Data = new uint8_t[Ty->getSize()];
  Mem.load(Result.Data, Address, Ty->getSize());
  return Result;
}

template <typename T> void Object::set(T Value, uint32_t Element)
{
  assert(Data);
  assert(Ty->isScalar() || Ty->isVector());
  assert(Ty->isScalar() ? (sizeof(T) == Ty->getSize() && Element == 0)
                        : sizeof(T) == Ty->getElementType()->getSize());
  ((T *)Data)[Element] = Value;
}

void Object::store(Memory &Mem, uint64_t Address) const
{
  assert(Data);
  Mem.store(Address, Ty->getSize(), Data);
}

void Object::zero() { memset(Data, 0, Ty->getSize()); }

// Explicit template instantiations for scalar types.
#define INSTANTIATE(TYPE)                                                      \
  template Object::Object(const talvos::Type *Ty, TYPE Value);                 \
  template TYPE Object::get(uint32_t) const;                                   \
  template void Object::set(TYPE, uint32_t)
INSTANTIATE(bool);
INSTANTIATE(int16_t);
INSTANTIATE(int32_t);
INSTANTIATE(int64_t);
INSTANTIATE(uint16_t);
INSTANTIATE(uint32_t);
INSTANTIATE(uint64_t);
INSTANTIATE(float);
INSTANTIATE(double);

} // namespace talvos
