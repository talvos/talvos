// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_OBJECT_H
#define TALVOS_OBJECT_H

#include <cassert>
#include <cstdint>
#include <cstring>

namespace talvos
{

class Memory;
class Type;

class Object
{
public:
  Object() { Data = nullptr; }

  template <typename T> static Object create(const Type *Ty, T Value)
  {
    Object Obj;
    Obj.Ty = Ty;
    Obj.Data = new uint8_t[sizeof(T)];
    *((T *)Obj.Data) = Value;
    return Obj;
  }

  Object clone() const
  {
    Object Obj;
    // TODO: Use size of this object
    Obj.Ty = this->Ty;
    Obj.Data = new uint8_t[4];
    memcpy(Obj.Data, this->Data, 4);
    return Obj;
  }

  void destroy() { delete[] Data; }

  template <typename T> T get() const
  {
    // TODO: assert size
    assert(Data);
    return *((T *)Data);
  }

  const Type *getType() const { return Ty; };

  bool isSet() const { return Data ? true : false; };

  static Object load(Memory *Mem, size_t Address);

  void store(Memory *Mem, size_t Address) const;

private:
  // TODO: Type, size, etc
  const Type *Ty;
  uint8_t *Data;
};

} // namespace talvos

#endif
