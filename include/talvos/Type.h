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

/// A list of types used for structure members.
/// The second element of each pair is the byte offset of the member.
typedef std::vector<std::pair<const Type *, uint32_t>> StructElementTypeList;

/// This class represents a SPIR-V type.
///
/// Factory methods are provided to create specific type instances. Some methods
/// of this class are only valid for certain types.
class Type
{
public:
  /// Returns the bit-width of this type.
  /// Valid for integer and floating point types.
  uint32_t getBitWidth() const;

  /// Returns the number of elements in this array, struct, or vector type.
  /// Returns 1 for any scalar type.
  uint32_t getElementCount() const { return ElementCount; }

  /// Returns the byte offset of the element at \p Index.
  /// Valid for array, pointer, runtime array, struct, and vector types.
  size_t getElementOffset(uint32_t Index) const;

  /// Returns the type of the element at \p Index.
  /// Valid for array, pointer, runtime array, struct, and vector types.
  /// For non-structure types, the value of \p Index is ignored.
  const Type *getElementType(uint32_t Index = 0) const;

  /// Returns the element type for vector types, or \p this for scalar types.
  /// Not valid for any other type.
  const Type *getScalarType() const;

  /// Returns the size of this type in bytes.
  size_t getSize() const;

  /// Returns the storage class of this type.
  /// Valid for pointer types.
  uint32_t getStorageClass() const;

  /// Returns \p true if this is an array, struct, or vector type.
  bool isComposite() const;

  /// Returns \p true if this is an integer type.
  bool isInt() const { return Id == INT; }

  /// Returns \p true if this is a scalar type.
  bool isScalar() const;

  /// Returns \p true if this is a vector type.
  bool isVector() const { return Id == VECTOR; }

  /// Create an array type.
  static std::unique_ptr<Type> getArray(const Type *ElemType,
                                        uint32_t ElementCount);

  /// Create a boolean type.
  static std::unique_ptr<Type> getBool();

  /// Create an integer type.
  static std::unique_ptr<Type> getInt(uint32_t Width);

  /// Create a function type.
  static std::unique_ptr<Type>
  getFunction(const Type *ReturnType,
              const std::vector<const Type *> &ArgTypes);

  /// Create a pointer type.
  static std::unique_ptr<Type> getPointer(uint32_t StorageClass,
                                          const Type *ElemType);

  /// Create a runtime array type.
  static std::unique_ptr<Type> getRuntimeArray(const Type *ElemType);

  /// Create a structure type.
  static std::unique_ptr<Type>
  getStruct(const StructElementTypeList &ElemTypes);

  /// Create a vector type.
  static std::unique_ptr<Type> getVector(const Type *ElemType,
                                         uint32_t ElemCount);

  /// Create a void type.
  static std::unique_ptr<Type> getVoid();

private:
  /// Create a new type.
  /// Used by the factory functions, which will populate class members directly.
  Type(uint32_t Id)
  {
    this->Id = Id;
    ElementCount = 1;
    ElementType = nullptr;
  };

  /// Identifiers that distinguish between different base types.
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

  uint32_t Id; ///< The ID of this type.

  uint32_t BitWidth; ///< Valid for integer and floating point types.

  uint32_t StorageClass; ///< Valid for pointer types.

  const Type *ElementType; ///< Valid for for pointer, array, and vector types.
  uint32_t ElementCount;   ///< Valid for array, struct, and vector types.
  StructElementTypeList ElementTypes; ///< Valid for struct types.

  const Type *ReturnType;                  ///< Valid for function types.
  std::vector<const Type *> ArgumentTypes; ///< Valid for function types.
};

} // namespace talvos

#endif
