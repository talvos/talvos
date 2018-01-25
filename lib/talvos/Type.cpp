// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "talvos/Type.h"

#include <cassert>

namespace talvos
{

size_t Type::getElementOffset(uint32_t Index) const
{
  if (Id == STRUCT)
    return ElementOffsets[Index];
  else
    return ElementType->getSize() * Index;
}

const Type *Type::getElementType(uint32_t Index) const
{
  if (Id == STRUCT)
  {
    assert(Index < ElementCount);
    return ElementTypes[Index];
  }
  else
    return ElementType;
}

size_t Type::getSize() const
{
  switch (Id)
  {
  case INT:
    return BitWidth / 8;
  case POINTER:
    return sizeof(size_t);
  case STRUCT:
    return ElementOffsets[ElementCount - 1] +
           ElementTypes[ElementCount - 1]->getSize();
  default:
    assert(false && "Type::getSize() not implemented for this Type");
    return 0;
  }
}

uint32_t Type::getStorageClass() const
{
  assert(Id == POINTER);
  return StorageClass;
}

Type *Type::getInt(uint32_t Width)
{
  Type *T = new Type(INT);
  T->BitWidth = Width;
  return T;
}

Type *Type::getPointer(uint32_t StorageClass, const Type *ElemType)
{
  Type *T = new Type(POINTER);
  T->StorageClass = StorageClass;
  T->ElementType = ElemType;
  return T;
}

Type *Type::getRuntimeArray(const Type *ElemType)
{
  Type *T = new Type(RUNTIME_ARRAY);
  T->ElementType = ElemType;
  return T;
}

Type *Type::getStruct(const std::vector<const Type *> &ElemTypes)
{
  Type *T = new Type(STRUCT);
  T->ElementTypes = ElemTypes;
  T->ElementCount = ElemTypes.size();
  T->ElementOffsets.resize(T->ElementCount);
  // TODO: Handle offsets to be specified via OpMemberDecorate
  T->ElementOffsets[0] = 0;
  for (int i = 1; i < T->ElementCount; i++)
  {
    T->ElementOffsets[i] =
        T->ElementOffsets[i - 1] + ElemTypes[i - 1]->getSize();
  }
  return T;
}

Type *Type::getVector(const Type *ElemType, uint32_t ElemCount)
{
  Type *T = new Type(VECTOR);
  T->ElementType = ElemType;
  T->ElementCount = ElemCount;
  return T;
}

Type *Type::getVoid() { return new Type(VOID); }

} // namespace talvos
