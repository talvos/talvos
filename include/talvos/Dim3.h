// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_DIM3_H
#define TALVOS_DIM3_H

#include <cstdint>

namespace talvos
{

/// Class representing a 3-dimensional size or ID.
///
/// Instances of this class provide element access via .{X, Y, Z} notation or
/// via the uint32_t Data[3] member.
class Dim3
{
public:
  /// Data members.
  union
  {
    struct
    {
      uint32_t X, Y, Z;
    };
    uint32_t Data[3];
  };

  /// Construct a Dim3 with values (1,1,1).
  Dim3() : X(1), Y(1), Z(1) {}

  /// Construct a Dim3 from specific values.
  Dim3(uint32_t X, uint32_t Y, uint32_t Z) : X(X), Y(Y), Z(Z) {}

  /// Construct a Dim3 from an array.
  Dim3(const uint32_t Data[3]) : X(Data[0]), Y(Data[1]), Z(Data[2]) {}

  /// Returns the component-wise addition of this Dim3 with \p D.
  Dim3 operator+(const Dim3 &D) const { return {X + D.X, Y + D.Y, Z + D.Z}; }

  /// Returns the component-wise multiplication of this Dim3 with \p D.
  Dim3 operator*(const Dim3 &D) const { return {X * D.X, Y * D.Y, Z * D.Z}; }
};

} // namespace talvos

#endif
