// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Object.h
/// This file declares the Object class.

#ifndef TALVOS_OBJECT_H
#define TALVOS_OBJECT_H

#include <cstdint>
#include <iosfwd>

#include "talvos/Type.h"

namespace talvos
{

class Memory;

/// This class represents an instruction result.
///
/// Instances of this class have a Type and a backing data store.
class Object
{
public:
  /// Create an empty, uninitialized object.
  Object() { Data = nullptr; }

  /// Allocate an object with type \p Ty.
  /// If \p Data is nullptr, the object data will be left uninitialized.
  /// Otherwise, the object data will be copied from \p Data.
  Object(const Type *Ty, const uint8_t *Data = nullptr);

  /// Allocate an object with type \p Ty, initializing it with \p Value.
  /// \p Ty must be a scalar type, and its size must match \p sizeof(T).
  template <typename T> Object(const Type *Ty, T Value);

  /// Destroy this object.
  ~Object();

  /// Copy-construct a new object, cloning the data from \p Src.
  Object(const Object &Src);

  /// Copy-assign to this object, cloning the data from \p Src.
  Object &operator=(const Object &Src);

  /// Move-construct an object, taking the data from \p Src.
  Object(Object &&Src) noexcept;

  /// Extract an element from a composite object.
  /// \returns a new object with the type and data of the target element.
  Object extract(const std::vector<uint32_t> &Indices) const;

  /// Get the value of this object as a scalar of type \p T.
  /// The type of this object must be either a scalar or a vector, and the size
  /// of the scalar type must match \p sizeof(T).
  template <typename T> T get(uint32_t Element = 0) const;

  /// Returns the type of this object.
  const Type *getType() const { return Ty; }

  /// Insert the value of \p Element into a composite object.
  void insert(const std::vector<uint32_t> &Indices, const Object &Element);

  /// Returns true if this object has been allocated.
  operator bool() const { return Data ? true : false; }

  /// Allow an Object to be inserted into an output stream.
  /// Converts the value of this object to a human readable format.
  friend std::ostream &operator<<(std::ostream &Stream, const Object &O);

  /// Set the value of this object to a scalar of type \p T.
  /// The type of this object must be either a scalar or a vector, and the size
  /// of the scalar type must match \p sizeof(T).
  template <typename T> void set(T Value, uint32_t Element = 0);

  /// Store the value of this object to memory at \p Address.
  void store(Memory &Mem, uint64_t Address) const;

  /// Set all of the value bits in this object to zero.
  void zero();

  /// Create an object of type \p Ty from the data at \p Address.
  static Object load(const Type *Ty, const Memory &Mem, uint64_t Address);

private:
  const Type *Ty; ///< The type of this object.
  uint8_t *Data;  ///< The raw data backing this object.
};

} // namespace talvos

#endif
