// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Device.h
/// This file declares the Device class.

#ifndef TALVOS_DEVICE_H
#define TALVOS_DEVICE_H

#include <memory>
#include <vector>

namespace talvos
{

class Command;
class Instruction;
class Invocation;
class Memory;
class PipelineExecutor;
class Plugin;
class Workgroup;

/// A Device instance encapsulates properties and state for the virtual device.
class Device
{
public:
  Device();
  ~Device();

  // Do not allow Device objects to be copied.
  ///\{
  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;
  ///\}

  /// Get the global memory instance associated with this device.
  Memory &getGlobalMemory() { return *GlobalMemory; }

  /// Returns the PipelineExecutor for this device.
  PipelineExecutor &getPipelineExecutor() { return *Executor; }

  /// Returns true if all of the loaded plugins are thread-safe.
  bool isThreadSafe() const;

  /// Report an error that has occurred during emulation.
  /// This prints \p Error to stderr along with the current execution context.
  /// If \p Fatal is true, abort() will be called after handling the error.
  void reportError(const std::string &Error, bool Fatal = false);

  /// \name Plugin notification functions.
  ///@{
  void reportCommandBegin(const Command *Cmd);
  void reportCommandComplete(const Command *Cmd);
  void reportInstructionExecuted(const Invocation *Invoc,
                                 const Instruction *Inst);
  void reportInvocationBegin(const Invocation *Invoc);
  void reportInvocationComplete(const Invocation *Invoc);
  void reportMemoryLoad(const Memory *Mem, uint64_t Address, uint64_t NumBytes);
  void reportMemoryMap(const Memory *Mem, uint64_t Base, uint64_t Offset,
                       uint64_t NumBytes);
  void reportMemoryStore(const Memory *Mem, uint64_t Address, uint64_t NumBytes,
                         const uint8_t *Data);
  void reportMemoryUnmap(const Memory *Mem, uint64_t Base);
  void reportWorkgroupBegin(const Workgroup *Group);
  void reportWorkgroupBarrier(const Workgroup *Group);
  void reportWorkgroupComplete(const Workgroup *Group);
  ///@}

private:
  Memory *GlobalMemory; ///< The global memory of this device.

  /// List of plugins that are currently loaded.
  std::vector<std::pair<void *, Plugin *>> Plugins;

  /// The pipeline executor instance.
  PipelineExecutor *Executor;

  /// The maximum number of errors to report.
  size_t MaxErrors;
};

} // namespace talvos

#endif
