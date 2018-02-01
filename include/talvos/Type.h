// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_TYPE_H
#define TALVOS_TYPE_H

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace talvos
{

class Type;

typedef std::vector<std::pair<const Type *, uint32_t>> StructElementTypeList;

class Type
{
public:
  uint32_t getBitWidth() const;
  uint32_t getElementCount() const { return ElementCount; }
  size_t getElementOffset(uint32_t Index) const;
  const Type *getElementType(uint32_t Index = 0) const;
  const Type *getScalarType() const;
  size_t getSize() const;
  uint32_t getStorageClass() const;
  bool isComposite() const;
  bool isInt() const { return Id == INT; }
  bool isScalar() const;
  bool isVector() const { return Id == VECTOR; }

  static std::unique_ptr<Type> getArray(const Type *ElemType,
                                        uint32_t ElementCount);
  static std::unique_ptr<Type> getBool();
  static std::unique_ptr<Type> getInt(uint32_t Width);
  static std::unique_ptr<Type>
  getFunction(const Type *ReturnType,
              const std::vector<const Type *> &ArgTypes);
  static std::unique_ptr<Type> getPointer(uint32_t StorageClass,
                                          const Type *ElemType);
  static std::unique_ptr<Type> getRuntimeArray(const Type *ElemType);
  static std::unique_ptr<Type>
  getStruct(const StructElementTypeList &ElemTypes);
  static std::unique_ptr<Type> getVector(const Type *ElemType,
                                         uint32_t ElemCount);
  static std::unique_ptr<Type> getVoid();

private:
  Type(uint32_t Id)
  {
    this->Id = Id;
    ElementCount = 1;
  };

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
