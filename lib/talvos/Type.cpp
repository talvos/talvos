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
    return ElementTypes[Index].second;
  else
    return ElementType->getSize() * Index;
}

const Type *Type::getElementType(uint32_t Index) const
{
  if (Id == STRUCT)
  {
    assert(Index < ElementCount);
    return ElementTypes[Index].first;
  }
  else
    return ElementType;
}

size_t Type::getSize() const
{
  switch (Id)
  {
  case ARRAY:
  case VECTOR:
    return ElementCount * ElementType->getSize();
  case INT:
    return BitWidth / 8;
  case POINTER:
    return sizeof(size_t);
  case STRUCT:
    return ElementTypes[ElementCount - 1].second +
           ElementTypes[ElementCount - 1].first->getSize();
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

Type *Type::getArray(const Type *ElemType, uint32_t ElementCount)
{
  Type *T = new Type(ARRAY);
  T->ElementType = ElemType;
  T->ElementCount = ElementCount;
  return T;
}

Type *Type::getInt(uint32_t Width)
{
  Type *T = new Type(INT);
  T->BitWidth = Width;
  return T;
}

Type *Type::getFunction(const Type *ReturnType,
                        const std::vector<const Type *> &ArgTypes)
{
  Type *T = new Type(FUNCTION);
  T->ReturnType = ReturnType;
  T->ArgumentTypes = ArgTypes;
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

Type *Type::getStruct(const StructElementTypeList &ElemTypes)
{
  Type *T = new Type(STRUCT);
  T->ElementTypes = ElemTypes;
  T->ElementCount = ElemTypes.size();
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
