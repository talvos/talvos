// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Plugin.h
/// This file declares the Plugin class.

#ifndef TALVOS_PLUGIN_H
#define TALVOS_PLUGIN_H

#include <cstdint>

namespace talvos
{

class Command;
class Instruction;
class Invocation;
class Memory;
class Workgroup;

/// Base class for Talvos plugins.
/// Plugins should extend this class and override the callbacks that they are
/// interested in.
class Plugin
{
public:
  virtual ~Plugin() = default;

  /// Returns true if the plugin is thread-safe.
  virtual bool isThreadSafe() const { return true; }

  /// Called when memory is atomically accessed by an instruction.
  virtual void atomicAccess(const Memory *Mem, uint64_t Address,
                            uint64_t NumBytes, uint32_t Opcode, uint32_t Scope,
                            uint32_t Semantics, const Invocation *Invoc)
  {}

  /// Called when a command has begun executing.
  virtual void commandBegin(const Command *Cmd) {}

  /// Called when a command has completed.
  virtual void commandComplete(const Command *Cmd) {}

  /// Called when the host loads data from memory.
  virtual void hostMemoryLoad(const Memory *Mem, uint64_t Address,
                              uint64_t NumBytes)
  {}

  /// Called when the host stores data to memory.
  virtual void hostMemoryStore(const Memory *Mem, uint64_t Address,
                               uint64_t NumBytes, const uint8_t *Data)
  {}

  /// Called when an instruction has been executed by an invocation.
  virtual void instructionExecuted(const Invocation *Invoc,
                                   const Instruction *Inst)
  {}

  /// Called when an invocation has begun executing.
  virtual void invocationBegin(const Invocation *Invoc) {}

  /// Called when an invocation has completed.
  virtual void invocationComplete(const Invocation *Invoc) {}

  /// Called when an invocation loads data from memory.
  virtual void memoryLoad(const Memory *Mem, uint64_t Address,
                          uint64_t NumBytes, const Invocation *Invoc)
  {}

  /// Called when a memory region is mapped by the host.
  virtual void memoryMap(const Memory *Mem, uint64_t Base, uint64_t Offset,
                         uint64_t NumBytes)
  {}

  /// Called when an invocation stores data to memory.
  virtual void memoryStore(const Memory *Mem, uint64_t Address,
                           uint64_t NumBytes, const uint8_t *Data,
                           const Invocation *Invoc)
  {}

  /// Called when a memory region is unmapped by the host.
  virtual void memoryUnmap(const Memory *Mem, uint64_t Base) {}

  /// Called when a workgroup has begun executing.
  virtual void workgroupBegin(const Workgroup *Group) {}

  /// Called when a workgroup barrier is cleared.
  virtual void workgroupBarrier(const Workgroup *Group) {}

  /// Called when a workgroup has completed.
  virtual void workgroupComplete(const Workgroup *Group) {}
};

} // namespace talvos

#endif
