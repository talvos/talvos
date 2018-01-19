// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_MEMORY_H
#define TALVOS_MEMORY_H

#include <cstdint>
#include <cstring>

namespace talvos
{

class Memory
{
public:
  void load(uint8_t *Result, size_t Address, size_t NumBytes);
  void store(size_t Address, size_t NumBytes, const uint8_t *Data);
};

} // namespace talvos

#endif
