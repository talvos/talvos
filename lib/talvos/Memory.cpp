// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <iostream>

#include "talvos/Memory.h"

namespace talvos
{

void Memory::load(uint8_t *Data, size_t Address, size_t NumBytes)
{
  // TODO: Perform load
  std::cout << "Loading " << NumBytes << " bytes from " << Address << std::endl;
}

void Memory::store(size_t Address, size_t NumBytes, const uint8_t *Data)
{
  // TODO: Perform store
  std::cout << "Storing " << NumBytes << " bytes to " << Address << std::endl;
}

} // namespace talvos
