// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Memory.h
/// This file declares the Memory class.

#ifndef TALVOS_MEMORY_H
#define TALVOS_MEMORY_H

#include <cstdint>
#include <cstring>
#include <vector>

namespace talvos
{

class Device;

/// Describes the scope of a memory instance.
enum class MemoryScope
{
  Device,
  Workgroup,
  Invocation
};

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
  Memory(Device &D, MemoryScope Scope);

  ~Memory();

  // Do not allow Memory objects to be copied.
  ///\{
  Memory(const Memory &) = delete;
  Memory &operator=(const Memory &) = delete;
  ///\}

  /// Allocate a new buffer of size \p NumBytes.
  /// \returns the virtual base address of the allocation.
  uint64_t allocate(uint64_t NumBytes);

  /// Dump the entire contents of this memory to stdout.
  void dump() const;

  /// Dump the contents of the buffer with base address \p Address to stdout.
  void dump(uint64_t Address) const;

  /// Get the scope of this memory instance.
  MemoryScope getScope() const { return Scope; }

  /// Load \p NumBytes of data from \p Address into \p Result.
  void load(uint8_t *Result, uint64_t Address, uint64_t NumBytes) const;

  /// Map a region of memory and return a pointer to it.
  uint8_t *map(uint64_t Base, uint64_t Offset, uint64_t NumBytes);

  /// Release the allocation with base address \p Address.
  void release(uint64_t Address);

  /// Store \p NumBytes of data from \p Data to \p Address.
  void store(uint64_t Address, uint64_t NumBytes, const uint8_t *Data);

  /// Unmap a previously mapped region of memory.
  void unmap(uint64_t Base);

  /// Copy data between memory instances.
  /// \p DstMem and \p SrcMem can be the same memory instance.
  static void copy(uint64_t DstAddress, Memory &DstMem, uint64_t SrcAddress,
                   const Memory &SrcMem, uint64_t NumBytes);

  /// Returns the string representation of \p Scope.
  static const char *scopeToString(MemoryScope Scope)
  {
    switch (Scope)
    {
    case MemoryScope::Device:
      return "Device";
    case MemoryScope::Workgroup:
      return "Workgroup";
    case MemoryScope::Invocation:
      return "Invocation";
    default:
      return "<invalid>";
    }
  }

private:
  Device &Dev; ///< The device this memory instance is part of.

  MemoryScope Scope; ///< The scope of this memory instance.

  /// An allocation within this memory instance.
  struct Buffer
  {
    uint64_t NumBytes; ///< The size of the allocation in bytes.
    uint8_t *Data;     ///< The raw data backing the allocation.
  };
  std::vector<Buffer> Buffers;       ///< List of allocations.
  std::vector<uint64_t> FreeBuffers; ///< Base addresses available for reuse.

  /// Check whether an access resides in an allocated region of memory.
  bool isAccessValid(uint64_t Address, uint64_t NumBytes) const;
};

} // namespace talvos

#endif
