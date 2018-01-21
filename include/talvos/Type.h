// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_TYPE_H
#define TALVOS_TYPE_H

#include <cstdint>
#include <vector>

namespace talvos
{

class Type
{
public:
  uint32_t getStorageClass() const;

  static Type *getInt(uint32_t Width);
  static Type *getPointer(uint32_t StorageClass, const Type *ElemType);
  static Type *getRuntimeArray(const Type *ElemType);
  static Type *getStruct(const std::vector<const Type *> &ElemTypes);
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
  };
  uint32_t Id;

  // Valid for integer types.
  uint32_t BitWidth;

  // Valid for pointer type.
  uint32_t StorageClass;

  // Valid for pointer, array, and vector types.
  const Type *ElementType;

  // Valid for struct type.
  std::vector<const Type *> ElementTypes;

  // Valid for vector types.
  uint32_t ElementCount;
};

} // namespace talvos

#endif
