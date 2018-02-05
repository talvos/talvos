// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "talvos/Type.h"

#include <cassert>

namespace talvos
{

uint32_t Type::getBitWidth() const
{
  assert(isInt());
  return BitWidth;
}

size_t Type::getElementOffset(uint32_t Index) const
{
  if (Id == STRUCT)
    return ElementTypes[Index].second;
  else
  {
    assert(ElementType && "Not an aggregate type");
    return ElementType->getSize() * Index;
  }
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

const Type *Type::getScalarType() const
{
  if (Id == VECTOR)
    return ElementType;
  else
  {
    assert(isScalar());
    return this;
  }
}

size_t Type::getSize() const
{
  switch (Id)
  {
  case ARRAY:
  case VECTOR:
    return ElementCount * ElementType->getSize();
  case BOOL:
    return 1;
  case INT:
    return BitWidth / 8;
  case POINTER:
    return sizeof(uint64_t);
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

bool Type::isComposite() const
{
  return (Id == ARRAY || Id == STRUCT || Id == VECTOR);
}

bool Type::isScalar() const
{
  return (Id == BOOL) || (Id == INT) || (Id == FLOAT) || (Id == POINTER);
}

std::unique_ptr<Type> Type::getArray(const Type *ElemType,
                                     uint32_t ElementCount)
{
  std::unique_ptr<Type> T(new Type(ARRAY));
  T->ElementType = ElemType;
  T->ElementCount = ElementCount;
  return T;
}

std::unique_ptr<Type> Type::getBool()
{
  return std::unique_ptr<Type>(new Type(BOOL));
}

std::unique_ptr<Type> Type::getInt(uint32_t Width)
{
  std::unique_ptr<Type> T(new Type(INT));
  T->BitWidth = Width;
  return T;
}

std::unique_ptr<Type>
Type::getFunction(const Type *ReturnType,
                  const std::vector<const Type *> &ArgTypes)
{
  std::unique_ptr<Type> T(new Type(FUNCTION));
  T->ReturnType = ReturnType;
  T->ArgumentTypes = ArgTypes;
  return T;
}

std::unique_ptr<Type> Type::getPointer(uint32_t StorageClass,
                                       const Type *ElemType)
{
  std::unique_ptr<Type> T(new Type(POINTER));
  T->StorageClass = StorageClass;
  T->ElementType = ElemType;
  return T;
}

std::unique_ptr<Type> Type::getRuntimeArray(const Type *ElemType)
{
  std::unique_ptr<Type> T(new Type(RUNTIME_ARRAY));
  T->ElementType = ElemType;
  return T;
}

std::unique_ptr<Type> Type::getStruct(const StructElementTypeList &ElemTypes)
{
  std::unique_ptr<Type> T(new Type(STRUCT));
  T->ElementTypes = ElemTypes;
  T->ElementCount = ElemTypes.size();
  return T;
}

std::unique_ptr<Type> Type::getVector(const Type *ElemType, uint32_t ElemCount)
{
  assert(ElemType->isScalar());
  std::unique_ptr<Type> T(new Type(VECTOR));
  T->ElementType = ElemType;
  T->ElementCount = ElemCount;
  return T;
}

std::unique_ptr<Type> Type::getVoid()
{
  return std::unique_ptr<Type>(new Type(VOID));
}

} // namespace talvos
