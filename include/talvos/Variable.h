// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Variable.h
/// This file declares the Variable class.

#ifndef TALVOS_VARIABLE_H
#define TALVOS_VARIABLE_H

#include <cstdint>
#include <map>

namespace talvos
{

class Type;

/// This class represents a module-scope variable declaration.
class Variable
{
public:
  /// Create a module scope variable with an optional initializer.
  Variable(uint32_t Id, const Type *Ty, uint32_t Initializer = 0);

  /// Add a decoration to this variable.
  void addDecoration(uint32_t Decoration, uint32_t Data);

  /// Returns the decoration data associated with this variable.
  /// \p Decoration must be a valid decoration that is present on this variable.
  uint32_t getDecoration(uint32_t Decoration) const;

  /// Returns the result ID of this variable.
  uint32_t getId() const { return Id; }

  /// Returns the result ID of this variables initializer, or 0 if not present.
  uint32_t getInitializer() const { return Initializer; }

  /// Returns the type of this variable.
  const Type *getType() const { return Ty; }

  /// Returns true if this variable has been decorated with \p Decoration.
  bool hasDecoration(uint32_t Decoration) const;

  /// Returns true if this variable has a buffer or uniform storage class.
  bool isBufferVariable() const;

private:
  /// Result ID of the variable.
  uint32_t Id;

  /// Type of the variable.
  const Type *Ty;

  /// Result ID of the initializer.
  uint32_t Initializer;

  /// Map of decorations.
  std::map<uint32_t, uint32_t> Decorations;
};

} // namespace talvos

#endif
