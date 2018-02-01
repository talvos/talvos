// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_MEMORY_H
#define TALVOS_MEMORY_H

#include <cstdint>
#include <cstring>
#include <vector>

namespace talvos
{

class Memory
{
public:
  Memory();
  size_t allocate(size_t NumBytes);
  void dump() const;
  void dump(size_t Address) const;

  /// Load \p NumBytes of data from \p Address into \p Result.
  void load(uint8_t *Result, size_t Address, size_t NumBytes) const;

  void release(size_t Address);
  void store(size_t Address, size_t NumBytes, const uint8_t *Data);

private:
  struct Buffer
  {
    size_t NumBytes;
    uint8_t *Data;
  };
  std::vector<Buffer> Buffers;
  std::vector<size_t> FreeBuffers;
};

} // namespace talvos

#endif
