// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Type.h
/// This file declares the Type class.

#ifndef TALVOS_TYPE_H
#define TALVOS_TYPE_H

#include <cstdint>
#include <cstring>
#include <iosfwd>
#include <memory>
#include <vector>

namespace talvos
{

class Type;

/// A list of types used for structure members.
/// The second element of each pair is the byte offset of the member.
typedef std::vector<std::pair<const Type *, uint64_t>> StructElementTypeList;

/// This class represents a SPIR-V type.
///
/// Factory methods are provided to create specific type instances. Some methods
/// of this class are only valid for certain types.
class Type
{
public:
  /// Identifiers that distinguish between different base types.
  enum TypeId
  {
    VOID,
    BOOL,
    INT,
    FLOAT,
    VECTOR,
    MATRIX,
    IMAGE,
    SAMPLER,
    SAMPLED_IMAGE,
    ARRAY,
    RUNTIME_ARRAY,
    STRUCT,
    POINTER,
    FUNCTION,
  };

  /// Returns the bit-width of this type.
  /// Valid for integer and floating point types.
  uint32_t getBitWidth() const;

  /// Returns the number of elements in this array, struct, or vector type.
  /// Returns 1 for any scalar type.
  uint32_t getElementCount() const { return ElementCount; }

  /// Returns the byte offset of the element at \p Index.
  /// Valid for array, pointer, runtime array, struct, and vector types.
  size_t getElementOffset(uint64_t Index) const;

  /// Returns the type of the element at \p Index.
  /// Valid for array, pointer, runtime array, struct, and vector types.
  /// For non-structure types, the value of \p Index is ignored.
  const Type *getElementType(uint64_t Index = 0) const;

  /// Returns the element type for vector types, or \p this for scalar types.
  /// Not valid for any other type.
  const Type *getScalarType() const;

  /// Returns the size of this type in bytes.
  /// Returns zero for void, function, and runtime array types.
  size_t getSize() const { return ByteSize; };

  /// Returns the storage class of this type.
  /// Valid for pointer types.
  uint32_t getStorageClass() const;

  /// Returns the type ID of this type.
  TypeId getTypeId() const { return Id; }

  /// Returns \p true if this is a bool type.
  bool isBool() const { return Id == BOOL; }

  /// Returns \p true if this is an array, struct, or vector type.
  bool isComposite() const;

  /// Returns \p true if this is a floating point type.
  bool isFloat() const { return Id == FLOAT; }

  /// Returns \p true if this is an integer type.
  bool isInt() const { return Id == INT; }

  /// Returns \p true if this is a pointer type.
  bool isPointer() const { return Id == POINTER; }

  /// Returns \p true if this is a scalar type.
  bool isScalar() const;

  /// Returns \p true if this is a vector type.
  bool isVector() const { return Id == VECTOR; }

  /// Allow a Type to be inserted into an output stream.
  /// Converts the type to a human readable format.
  friend std::ostream &operator<<(std::ostream &Stream, const Type *Ty);

  /// Create an array type.
  static std::unique_ptr<Type>
  getArray(const Type *ElemType, uint32_t ElementCount, uint32_t ArrayStride);

  /// Create a boolean type.
  static std::unique_ptr<Type> getBool();

  /// Create a floating point type.
  static std::unique_ptr<Type> getFloat(uint32_t Width);

  /// Create an integer type.
  static std::unique_ptr<Type> getInt(uint32_t Width);

  /// Create a function type.
  static std::unique_ptr<Type>
  getFunction(const Type *ReturnType,
              const std::vector<const Type *> &ArgTypes);

  /// Create an image type.
  static std::unique_ptr<Type> getImage(const Type *SampledType, uint32_t Dim,
                                        uint32_t Depth, bool Arrayed, bool MS,
                                        uint32_t Sampled, uint32_t Format);

  /// Create a matrix type.
  static std::unique_ptr<Type> getMatrix(const Type *ColumnType,
                                         uint32_t NumColumns);

  /// Create a pointer type.
  static std::unique_ptr<Type>
  getPointer(uint32_t StorageClass, const Type *ElemType, uint32_t ArrayStride);

  /// Create a runtime array type.
  static std::unique_ptr<Type> getRuntimeArray(const Type *ElemType,
                                               uint32_t ArrayStride);

  /// Create a sampled image type
  static std::unique_ptr<Type> getSampledImage(const Type *ImageType);

  /// Create a sampler type.
  static std::unique_ptr<Type> getSampler();

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
  Type(TypeId Id, size_t ByteSize)
  {
    this->Id = Id;
    this->ByteSize = ByteSize;
    ElementCount = 1;
    ElementType = nullptr;
  };

  TypeId Id; ///< The ID of this type.

  size_t ByteSize; ///< The size of this type in bytes.

  uint32_t BitWidth; ///< Valid for integer and floating point types.

  uint32_t StorageClass; ///< Valid for pointer types.

  const Type *ElementType; ///< Valid for pointer and composite types.
  uint32_t ElementCount;   ///< Valid for composite types.
  uint32_t ArrayStride;    ///< Valid for array and pointer types.
  StructElementTypeList ElementTypes; ///< Valid for struct types.

  const Type *ReturnType;                  ///< Valid for function types.
  std::vector<const Type *> ArgumentTypes; ///< Valid for function types.

  uint32_t Dimensionality; ///< Valid for image types.
  uint32_t Depth;          ///< Valid for image types.
  bool Arrayed;            ///< Valid for image types.
  bool Multisampled;       ///< Valid for image types.
  uint32_t Sampled;        ///< Valid for image types.
  uint32_t Format;         ///< Valid for image types.
};

} // namespace talvos

#endif
