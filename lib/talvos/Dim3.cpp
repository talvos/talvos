// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <iostream>

#include "talvos/Dim3.h"

namespace talvos
{

std::ostream &operator<<(std::ostream &Stream, const Dim3 &D)
{
  return Stream << std::dec << "(" << D.X << "," << D.Y << "," << D.Z << ")";
}

} // namespace talvos
