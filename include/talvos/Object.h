// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_OBJECT_H
#define TALVOS_OBJECT_H

#include <cassert>
#include <cstdint>
#include <cstring>

#include "talvos/Type.h"

namespace talvos
{

class Memory;

class Object
{
public:
  Object() { Data = nullptr; }

  static Object create(const Type *Ty)
  {
    assert(Ty);
    Object Result;
    Result.Ty = Ty;
    Result.Data = new uint8_t[Ty->getSize()];
    return Result;
  }

  template <typename T> static Object create(const Type *Ty, T Value)
  {
    assert(Ty->isScalar());
    assert(sizeof(T) == Ty->getSize());
    Object Result;
    Result.Ty = Ty;
    Result.Data = new uint8_t[sizeof(T)];
    *((T *)Result.Data) = Value;
    return Result;
  }

  static Object createComposite(const Type *Ty,
                                const std::vector<Object> &Elements);

  Object clone() const
  {
    assert(Data);
    Object Result;
    Result.Ty = this->Ty;
    Result.Data = new uint8_t[Ty->getSize()];
    memcpy(Result.Data, this->Data, Ty->getSize());
    return Result;
  }

  void destroy() { delete[] Data; }

  Object extract(const std::vector<uint32_t> &Indices) const;

  template <typename T> T get(uint32_t Element = 0) const
  {
    assert(Data);
    assert(Ty->isScalar() || Ty->isVector());
    assert(Ty->isScalar() ? (sizeof(T) == Ty->getSize() && Element == 0)
                          : sizeof(T) == Ty->getElementType()->getSize());
    return ((T *)Data)[Element];
  }

  const Type *getType() const { return Ty; }

  bool isSet() const { return Data ? true : false; }

  static Object load(const Type *Ty, Memory *Mem, size_t Address);

  template <typename T> void set(T Value, uint32_t Element = 0)
  {
    assert(Data);
    assert(Ty->isScalar() || Ty->isVector());
    assert(Ty->isScalar() ? (sizeof(T) == Ty->getSize() && Element == 0)
                          : sizeof(T) == Ty->getElementType()->getSize());
    ((T *)Data)[Element] = Value;
  }

  void store(Memory *Mem, size_t Address) const;

private:
  const Type *Ty;
  uint8_t *Data;
};

} // namespace talvos

#endif
