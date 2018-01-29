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

  template <typename T> static Object create(const Type *Ty, T Value)
  {
    assert(sizeof(T) == Ty->getSize());
    Object Obj;
    Obj.Ty = Ty;
    Obj.Data = new uint8_t[sizeof(T)];
    *((T *)Obj.Data) = Value;
    return Obj;
  }

  static Object createComposite(const Type *Ty,
                                const std::vector<Object> &Elements);

  Object clone() const
  {
    Object Obj;
    Obj.Ty = this->Ty;
    Obj.Data = new uint8_t[Ty->getSize()];
    memcpy(Obj.Data, this->Data, Ty->getSize());
    return Obj;
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

  const Type *getType() const { return Ty; };

  bool isSet() const { return Data ? true : false; };

  static Object load(const Type *Ty, Memory *Mem, size_t Address);

  void store(Memory *Mem, size_t Address) const;

private:
  const Type *Ty;
  uint8_t *Data;
};

} // namespace talvos

#endif
