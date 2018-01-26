// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_TYPE_H
#define TALVOS_TYPE_H

#include <cstdint>
#include <cstring>
#include <vector>

namespace talvos
{

class Type;

typedef std::vector<std::pair<const Type *, uint32_t>> StructElementTypeList;

class Type
{
public:
  size_t getElementOffset(uint32_t Index) const;
  const Type *getElementType(uint32_t Index = 0) const;
  size_t getSize() const;
  uint32_t getStorageClass() const;
  bool isComposite() const;

  static Type *getArray(const Type *ElemType, uint32_t ElementCount);
  static Type *getBool();
  static Type *getInt(uint32_t Width);
  static Type *getFunction(const Type *ReturnType,
                           const std::vector<const Type *> &ArgTypes);
  static Type *getPointer(uint32_t StorageClass, const Type *ElemType);
  static Type *getRuntimeArray(const Type *ElemType);
  static Type *getStruct(const StructElementTypeList &ElemTypes);
  static Type *getVector(const Type *ElemType, uint32_t ElemCount);
  static Type *getVoid();

private:
  Type(uint32_t Id) { this->Id = Id; };

  enum
  {
    VOID,
    BOOL,
    INT,
    FLOAT,
    VECTOR,
    ARRAY,
    RUNTIME_ARRAY,
    STRUCT,
    POINTER,
    FUNCTION,
  };
  uint32_t Id;

  // Valid for integer types.
  uint32_t BitWidth;

  // Valid for pointer type.
  uint32_t StorageClass;

  // Valid for pointer, array, and vector types.
  const Type *ElementType;

  // Valid for struct type.
  StructElementTypeList ElementTypes;

  // Valid for array, struct, and vector types.
  uint32_t ElementCount;

  // Valid for function types.
  const Type *ReturnType;
  std::vector<const Type *> ArgumentTypes;
};

} // namespace talvos

#endif
