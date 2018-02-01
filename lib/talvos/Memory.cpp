// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <cassert>
#include <iomanip>
#include <iostream>

#include "talvos/Memory.h"

// TODO: 32-bit?
// TODO: Allow different number of buffer bits depending on address space
#define BUFFER_BITS (16)
#define OFFSET_BITS (64 - BUFFER_BITS)

namespace talvos
{

Memory::Memory()
{
  // Skip the first buffer identifier (0).
  Buffers.resize(1);
}

size_t Memory::allocate(size_t NumBytes)
{
  // Allocate buffer.
  Buffer B;
  B.NumBytes = NumBytes;
  B.Data = new uint8_t[NumBytes];

  // Get the next available buffer identifier.
  size_t Id;
  if (FreeBuffers.size())
  {
    // Re-use previously released buffer identifier.
    Id = FreeBuffers.back();
    FreeBuffers.pop_back();
    Buffers[Id] = B;
  }
  else
  {
    // Allocate new buffer identifier.
    Id = Buffers.size();
    Buffers.push_back(B);
  }

  return (Id << OFFSET_BITS);
}

void Memory::dump() const
{
  for (size_t Id = 1; Id < Buffers.size(); Id++)
  {
    if (Buffers[Id].Data)
      dump(Id << OFFSET_BITS);
  }
}

void Memory::dump(size_t Address) const
{
  size_t Id = (Address >> OFFSET_BITS);

  if (!Buffers[Id].Data)
  {
    std::cerr << "Memory::dump() invalid address: " << Address << std::endl;
    return;
  }

  for (size_t i = 0; i < Buffers[Id].NumBytes; i++)
  {
    if (i % 4 == 0)
    {
      std::cout << std::endl
                << std::hex << std::uppercase << std::setw(16)
                << std::setfill(' ') << std::right
                << ((((size_t)Id) << OFFSET_BITS) | i) << ":";
    }
    std::cout << " " << std::hex << std::uppercase << std::setw(2)
              << std::setfill('0') << (int)Buffers[Id].Data[i];
  }
  std::cout << std::endl;
}

void Memory::load(uint8_t *Data, size_t Address, size_t NumBytes) const
{
  size_t Id = (Address >> OFFSET_BITS);
  size_t Offset = (Address & (((size_t)-1) >> BUFFER_BITS));

  // TODO: Generate useful error message for invalid memory accesses
  assert(Id < Buffers.size());
  assert(Buffers[Id].Data);
  assert((Offset + NumBytes) <= Buffers[Id].NumBytes);

  memcpy(Data, Buffers[Id].Data + Offset, NumBytes);
}

void Memory::release(size_t Address)
{
  size_t Id = (Address >> OFFSET_BITS);
  assert(Buffers[Id].Data != nullptr);

  // Release memory used by buffer.
  delete[] Buffers[Id].Data;
  Buffers[Id].Data = nullptr;

  FreeBuffers.push_back(Id);
}

void Memory::store(size_t Address, size_t NumBytes, const uint8_t *Data)
{
  size_t Id = (Address >> OFFSET_BITS);
  size_t Offset = (Address & (((size_t)-1) >> BUFFER_BITS));

  // TODO: Generate useful error message for invalid memory accesses
  assert(Id < Buffers.size());
  assert(Buffers[Id].Data);
  assert((Offset + NumBytes) <= Buffers[Id].NumBytes);

  memcpy(Buffers[Id].Data + Offset, Data, NumBytes);
}

} // namespace talvos
