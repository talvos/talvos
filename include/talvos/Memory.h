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

/// This class represents an address space in the virtual device.
///
/// This class provides methods to allocate and release buffers and access their
/// data. A virtual addressing scheme is used for memory addresses. Base
/// addresses for buffers are unique within an instance of this class, but not
/// across separate instances. It is therefore the responsibility of the caller
/// to route load/store calls to the correct Memory object.
class Memory
{
public:
  /// Create a new Memory instance.
  Memory();

  ~Memory();
  Memory(const Memory &) = delete;
  Memory &operator=(const Memory &) = delete;

  /// Allocate a new buffer of size \p NumBytes.
  /// \returns the virtual base address of the allocation.
  uint64_t allocate(uint64_t NumBytes);

  /// Dump the entire contents of this memory to stdout.
  void dump() const;

  /// Dump the contents of the buffer with base address \p Address to stdout.
  void dump(uint64_t Address) const;

  /// Load \p NumBytes of data from \p Address into \p Result.
  void load(uint8_t *Result, uint64_t Address, uint64_t NumBytes) const;

  /// Release the allocation with base address \p Address.
  void release(uint64_t Address);

  /// Store \p NumBytes of data from \p Data to \p Address.
  void store(uint64_t Address, uint64_t NumBytes, const uint8_t *Data);

private:
  /// An allocation within this memory instance.
  struct Buffer
  {
    uint64_t NumBytes; ///< The size of the allocation in bytes.
    uint8_t *Data;   ///< The raw data backing the allocation.
  };
  std::vector<Buffer> Buffers;       ///< List of allocations.
  std::vector<uint64_t> FreeBuffers; ///< Base addresses available for reuse.
};

} // namespace talvos

#endif
