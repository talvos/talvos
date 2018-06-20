// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Type.cpp
/// This file defines the Type class.

#include <spirv/unified1/spirv.h>

#include "talvos/Type.h"

#include <cassert>
#include <iostream>

namespace talvos
{

uint32_t Type::getBitWidth() const
{
  assert(isInt() || isFloat());
  return BitWidth;
}

size_t Type::getElementOffset(uint64_t Index) const
{
  if (Id == STRUCT)
    return ElementOffsets[Index];
  else if (Id == VECTOR || Id == MATRIX)
    return ElementType->getSize() * Index;
  else if (Id == ARRAY || Id == POINTER || Id == RUNTIME_ARRAY)
    return ArrayStride * Index;
  assert(false && "Not an aggregate type");
  abort();
}

const Type *Type::getElementType(uint64_t Index) const
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

uint32_t Type::getStorageClass() const
{
  assert(Id == POINTER);
  return StorageClass;
}

const std::map<uint32_t, uint32_t> &
Type::getStructMemberDecorations(uint32_t Index) const
{
  return ElementTypes[Index].second;
}

bool Type::isComposite() const
{
  return (Id == ARRAY || Id == STRUCT || Id == VECTOR || Id == MATRIX);
}

bool Type::isScalar() const
{
  return (Id == BOOL) || (Id == INT) || (Id == FLOAT) || (Id == POINTER);
}

std::ostream &operator<<(std::ostream &Stream, const Type *Ty)
{
  switch (Ty->Id)
  {
  case Type::VOID:
    Stream << "void";
    break;
  case Type::BOOL:
    Stream << "bool";
    break;
  case Type::INT:
    Stream << "int" << Ty->BitWidth;
    break;
  case Type::FLOAT:
    Stream << "float" << Ty->BitWidth;
    break;
  case Type::VECTOR:
    Stream << Ty->ElementType << "v" << Ty->ElementCount;
    break;
  case Type::MATRIX:
    Stream << Ty->ElementType->ElementType << " mat" << Ty->ElementCount << "x"
           << Ty->ElementType->ElementCount;
    break;
  case Type::ARRAY:
    Stream << Ty->ElementType << "[" << Ty->ElementCount << "]";
    break;
  case Type::RUNTIME_ARRAY:
    Stream << Ty->ElementType << "[]";
    break;
  case Type::STRUCT:
  {
    Stream << "struct {";
    for (unsigned i = 0; i < Ty->ElementCount; i++)
    {
      if (i > 0)
        Stream << ",";
      Stream << Ty->ElementTypes[i].first;
      // TODO: Show member offsets?
    }
    Stream << "}";
    break;
  }
  case Type::POINTER:
    Stream << Ty->ElementType << "*";
    break;
  case Type::FUNCTION:
  {
    Stream << Ty->ReturnType << " function (";
    for (unsigned i = 0; i < Ty->ArgumentTypes.size(); i++)
    {
      if (i > 0)
        Stream << ",";
      Stream << Ty->ArgumentTypes[i];
    }
    Stream << ")";
    break;
  }
  default:
    Stream << "<unhandled type>";
    break;
  }
  return Stream;
}

std::unique_ptr<Type> Type::getArray(const Type *ElemType,
                                     uint32_t ElementCount,
                                     uint32_t ArrayStride)
{
  assert(ArrayStride >= ElemType->getSize());
  std::unique_ptr<Type> T(new Type(ARRAY, ElementCount * ArrayStride));
  T->ElementType = ElemType;
  T->ElementCount = ElementCount;
  T->ArrayStride = ArrayStride;
  return T;
}

std::unique_ptr<Type> Type::getBool()
{
  return std::unique_ptr<Type>(new Type(BOOL, 1));
}

std::unique_ptr<Type> Type::getFloat(uint32_t Width)
{
  std::unique_ptr<Type> T(new Type(FLOAT, Width / 8));
  T->BitWidth = Width;
  return T;
}

std::unique_ptr<Type> Type::getInt(uint32_t Width)
{
  std::unique_ptr<Type> T(new Type(INT, Width / 8));
  T->BitWidth = Width;
  return T;
}

std::unique_ptr<Type>
Type::getFunction(const Type *ReturnType,
                  const std::vector<const Type *> &ArgTypes)
{
  std::unique_ptr<Type> T(new Type(FUNCTION, 0));
  T->ReturnType = ReturnType;
  T->ArgumentTypes = ArgTypes;
  return T;
}

std::unique_ptr<Type> Type::getImage(const Type *SampledType, uint32_t Dim,
                                     uint32_t Depth, bool Arrayed, bool MS,
                                     uint32_t Sampled, uint32_t Format)
{
  std::unique_ptr<Type> T(new Type(IMAGE, sizeof(uint64_t)));
  T->ElementType = SampledType;
  T->Dimensionality = Dim;
  T->Depth = Depth;
  T->Arrayed = Arrayed;
  T->Multisampled = MS;
  T->Sampled = Sampled;
  T->Format = Format;
  return T;
}

std::unique_ptr<Type> Type::getMatrix(const Type *ColumnType,
                                      uint32_t NumColumns)
{
  std::unique_ptr<Type> T(new Type(MATRIX, NumColumns * ColumnType->getSize()));
  T->ElementType = ColumnType;
  T->ElementCount = NumColumns;
  return T;
}

std::unique_ptr<Type> Type::getPointer(uint32_t StorageClass,
                                       const Type *ElemType,
                                       uint32_t ArrayStride)
{
  assert(ArrayStride >= ElemType->getSize());
  std::unique_ptr<Type> T(new Type(POINTER, sizeof(uint64_t)));
  T->StorageClass = StorageClass;
  T->ElementType = ElemType;
  T->ArrayStride = ArrayStride;
  return T;
}

std::unique_ptr<Type> Type::getRuntimeArray(const Type *ElemType,
                                            uint32_t ArrayStride)
{
  assert(ArrayStride >= ElemType->getSize());
  std::unique_ptr<Type> T(new Type(RUNTIME_ARRAY, 0));
  T->ElementType = ElemType;
  T->ArrayStride = ArrayStride;
  return T;
}

std::unique_ptr<Type> Type::getSampledImage(const Type *ImageType)
{
  std::unique_ptr<Type> T(new Type(SAMPLED_IMAGE, sizeof(uint64_t)));
  T->ElementType = ImageType;
  return T;
}

std::unique_ptr<Type> Type::getSampler()
{
  return std::unique_ptr<Type>(new Type(SAMPLER, sizeof(uint64_t)));
}

std::unique_ptr<Type> Type::getStruct(const StructElementTypeList &ElemTypes)
{
  // Build list of member offsets, using Offset decoration if supplied.
  size_t CurrentOffset = 0;
  std::vector<size_t> Offsets(ElemTypes.size());
  for (size_t i = 0; i < ElemTypes.size(); i++)
  {
    if (ElemTypes[i].second.count(SpvDecorationOffset))
      Offsets[i] = ElemTypes[i].second.at(SpvDecorationOffset);
    else
      Offsets[i] = CurrentOffset;

    // Special case for size of a matrix with an explicit layout.
    if (ElemTypes[i].first->isMatrix() &&
        ElemTypes[i].second.count(SpvDecorationMatrixStride))
    {
      const Type *MatrixType = ElemTypes[i].first;
      const Type *VectorType = MatrixType->getElementType();
      uint32_t MatrixStride = ElemTypes[i].second.at(SpvDecorationMatrixStride);

      // Calculate size of matrix based on stride and layout.
      size_t MatrixSize;
      if (ElemTypes[i].second.count(SpvDecorationColMajor))
        MatrixSize = MatrixType->getElementCount() * MatrixStride;
      else
        MatrixSize = VectorType->getElementCount() * MatrixStride;

      CurrentOffset = Offsets[i] + MatrixSize;
    }
    else
      CurrentOffset = Offsets[i] + ElemTypes[i].first->getSize();
  }

  std::unique_ptr<Type> T(new Type(STRUCT, CurrentOffset));
  T->ElementTypes = ElemTypes;
  T->ElementOffsets = Offsets;
  T->ElementCount = (uint32_t)ElemTypes.size();
  return T;
}

std::unique_ptr<Type> Type::getVector(const Type *ElemType, uint32_t ElemCount)
{
  assert(ElemType->isScalar());
  std::unique_ptr<Type> T(new Type(VECTOR, ElemCount * ElemType->getSize()));
  T->ElementType = ElemType;
  T->ElementCount = ElemCount;
  return T;
}

std::unique_ptr<Type> Type::getVoid()
{
  return std::unique_ptr<Type>(new Type(VOID, 0));
}

} // namespace talvos
