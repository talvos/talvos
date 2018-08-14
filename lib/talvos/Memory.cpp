// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Memory.cpp
/// This file defines the Memory class.

#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "talvos/Device.h"
#include "talvos/Memory.h"

// TODO: Allow different number of buffer bits depending on address space

/// Number of bits used for the buffer ID.
#define BUFFER_BITS (16)

/// Number of bits used for the address offset.
#define OFFSET_BITS (64 - BUFFER_BITS)

namespace talvos
{

Memory::Memory(Device &D, MemoryScope Scope) : Dev(D), Scope(Scope)
{
  // Skip the first buffer identifier (0).
  Buffers.resize(1);
}

Memory::~Memory()
{
  // Release all allocations.
  for (size_t Id = 1; Id < Buffers.size(); Id++)
    delete[] Buffers[Id].Data;
}

uint64_t Memory::allocate(uint64_t NumBytes)
{
  std::lock_guard<std::mutex> Lock(Mutex);

  // Allocate buffer.
  Buffer B;
  B.NumBytes = NumBytes;
  B.Data = new uint8_t[NumBytes];

  // Get the next available buffer identifier.
  uint64_t Id;
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

void Memory::dump(uint64_t Address) const
{
  uint64_t Id = (Address >> OFFSET_BITS);

  if (!Buffers[Id].Data)
  {
    std::cerr << "Memory::dump() invalid address: " << Address << std::endl;
    return;
  }

  for (uint64_t i = 0; i < Buffers[Id].NumBytes; i++)
  {
    if (i % 4 == 0)
    {
      std::cout << std::endl
                << std::hex << std::uppercase << std::setw(16)
                << std::setfill(' ') << std::right
                << ((((uint64_t)Id) << OFFSET_BITS) | i) << ":";
    }
    std::cout << " " << std::hex << std::uppercase << std::setw(2)
              << std::setfill('0') << (int)Buffers[Id].Data[i];
  }
  std::cout << std::endl;
}

bool Memory::isAccessValid(uint64_t Address, uint64_t NumBytes) const
{
  uint64_t Id = (Address >> OFFSET_BITS);
  uint64_t Offset = (Address & (((uint64_t)-1) >> BUFFER_BITS));
  if (Id >= Buffers.size())
    return false;
  if (!Buffers[Id].Data)
    return false;
  if ((Offset + NumBytes) > Buffers[Id].NumBytes)
    return false;
  return true;
}

void Memory::load(uint8_t *Data, uint64_t Address, uint64_t NumBytes) const
{
  uint64_t Id = (Address >> OFFSET_BITS);
  uint64_t Offset = (Address & (((uint64_t)-1) >> BUFFER_BITS));

  Dev.reportMemoryLoad(this, Address, NumBytes);

  if (!isAccessValid(Address, NumBytes))
  {
    std::stringstream Err;
    Err << "Invalid load of " << NumBytes << " bytes"
        << " from address 0x" << std::hex << Address << " ("
        << scopeToString(Scope) << " scope) ";
    Dev.reportError(Err.str());

    // Zero output data to conform to robust buffer access feature.
    memset(Data, 0, NumBytes);

    return;
  }

  memcpy(Data, Buffers[Id].Data + Offset, NumBytes);
}

uint8_t *Memory::map(uint64_t Base, uint64_t Offset, uint64_t NumBytes)
{
  uint64_t Id = (Base >> OFFSET_BITS);

  Dev.reportMemoryMap(this, Base, Offset, NumBytes);

  if (!isAccessValid(Base + Offset, NumBytes))
  {
    std::stringstream Err;
    Err << "Invalid mapping of " << NumBytes << " bytes"
        << " from address 0x" << std::hex << (Base + Offset) << " ("
        << scopeToString(Scope) << " scope) ";
    Dev.reportError(Err.str());
    return nullptr;
  }

  return Buffers[Id].Data + Offset;
}

void Memory::release(uint64_t Address)
{
  std::lock_guard<std::mutex> Lock(Mutex);

  uint64_t Id = (Address >> OFFSET_BITS);
  assert(Buffers[Id].Data != nullptr);

  // Release memory used by buffer.
  delete[] Buffers[Id].Data;
  Buffers[Id].Data = nullptr;

  FreeBuffers.push_back(Id);
}

void Memory::store(uint64_t Address, uint64_t NumBytes, const uint8_t *Data)
{
  uint64_t Id = (Address >> OFFSET_BITS);
  uint64_t Offset = (Address & (((uint64_t)-1) >> BUFFER_BITS));

  Dev.reportMemoryStore(this, Address, NumBytes, Data);

  if (!isAccessValid(Address, NumBytes))
  {
    std::stringstream Err;
    Err << "Invalid store of " << NumBytes << " bytes"
        << " to address 0x" << std::hex << Address << " ("
        << scopeToString(Scope) << " scope) ";
    Dev.reportError(Err.str());
    return;
  }

  memcpy(Buffers[Id].Data + Offset, Data, NumBytes);
}

void Memory::unmap(uint64_t Base) { Dev.reportMemoryUnmap(this, Base); }

void Memory::copy(uint64_t DstAddress, Memory &DstMem, uint64_t SrcAddress,
                  const Memory &SrcMem, uint64_t NumBytes)
{
  uint64_t SrcId = (SrcAddress >> OFFSET_BITS);
  uint64_t SrcOffset = (SrcAddress & (((uint64_t)-1) >> BUFFER_BITS));

  SrcMem.Dev.reportMemoryLoad(&SrcMem, SrcAddress, NumBytes);

  if (!SrcMem.isAccessValid(SrcAddress, NumBytes))
  {
    std::stringstream Err;
    Err << "Invalid load of " << NumBytes << " bytes"
        << " from address 0x" << std::hex << SrcAddress << " ("
        << scopeToString(SrcMem.Scope) << " scope) ";
    SrcMem.Dev.reportError(Err.str());
    return;
  }

  DstMem.store(DstAddress, NumBytes, SrcMem.Buffers[SrcId].Data + SrcOffset);
}

} // namespace talvos
