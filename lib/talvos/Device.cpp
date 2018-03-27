// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Device.cpp
/// This file defines the Device class.

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>

#if defined(_WIN32) && !defined(__MINGW32__)
#include <windows.h>
#undef ERROR
#undef VOID
#else
#include <dlfcn.h>
#endif

#include "CommandInvocation.h"
#include "Utils.h"
#include "talvos/Device.h"
#include "talvos/DispatchCommand.h"
#include "talvos/Function.h"
#include "talvos/Instruction.h"
#include "talvos/Invocation.h"
#include "talvos/Memory.h"
#include "talvos/Module.h"
#include "talvos/Plugin.h"
#include "talvos/Workgroup.h"

namespace talvos
{

typedef Plugin *(*CreatePluginFunc)(const Device *);
typedef void (*DestroyPluginFunc)(Plugin *);

Device::Device()
{
  GlobalMemory = new Memory(*this, MemoryScope::Device);

  CurrentCommand = nullptr;

  // Load plugins from dynamic libraries.
  const char *PluginList = getenv("TALVOS_PLUGINS");
  if (PluginList)
  {
    std::istringstream SS(PluginList);
    std::string LibPath;
    while (std::getline(SS, LibPath, ';'))
    {
#if defined(_WIN32) && !defined(__MINGW32__)
      // Open plugin library file.
      HMODULE Library = LoadLibraryA(LibPath.c_str());
      if (!Library)
      {
        std::cerr << "Talvos: Failed to load plugin '" << LibPath << "' - "
                  << GetLastError() << std::endl;
        continue;
      }

      // Get handle to plugin creation function.
      void *Create = GetProcAddress(Library, "talvosCreatePlugin");
      if (!Create)
      {
        std::cerr << "Talvos: Failed to load plugin '" << LibPath << "' - "
                  << GetLastError() << std::endl;
        continue;
      }
#else
      // Open plugin library file.
      void *Library = dlopen(LibPath.c_str(), RTLD_NOW);
      if (!Library)
      {
        std::cerr << "Talvos: Failed to load plugin '" << LibPath << "' - "
                  << dlerror() << std::endl;
        continue;
      }

      // Get handle to plugin creation function.
      void *Create = dlsym(Library, "talvosCreatePlugin");
      if (!Create)
      {
        std::cerr << "Talvos: Failed to load plugin '" << LibPath << "' - "
                  << dlerror() << std::endl;
        continue;
      }
#endif

      // Create plugin and add to list.
      Plugin *P = ((CreatePluginFunc)Create)(this);
      Plugins.push_back({Library, P});
    }
  }
}

Device::~Device()
{
  // Destroy plugins and unload their dynamic libraries.
  for (auto P : Plugins)
  {
#if defined(_WIN32) && !defined(__MINGW32__)
    void *Destroy = GetProcAddress((HMODULE)P.first, "talvosDestroyPlugin");
    if (Destroy)
      ((DestroyPluginFunc)Destroy)(P.second);
    FreeLibrary((HMODULE)P.first);
#else
    void *Destroy = dlsym(P.first, "talvosDestroyPlugin");
    if (Destroy)
      ((DestroyPluginFunc)Destroy)(P.second);
    dlclose(P.first);
#endif
  }

  delete GlobalMemory;
}

bool Device::isThreadSafe() const
{
  for (auto P : Plugins)
    if (!P.second->isThreadSafe())
      return false;
  return true;
}

void Device::reportError(const std::string &Error, bool Fatal)
{
  // Guard output to avoid mangling error messages from multiple threads.
  static std::mutex ErrorMutex;
  std::lock_guard<std::mutex> Lock(ErrorMutex);

  std::cerr << std::endl;
  std::cerr << Error << std::endl;

  if (CurrentCommand && CurrentCommand->isWorkerThread())
  {
    // Show current entry point.
    const Module *Mod = CurrentCommand->getCommand().getModule();
    uint32_t EntryPointId = CurrentCommand->getCommand().getFunction()->getId();
    std::cerr << "    Entry point:";
    std::cerr << " %" << EntryPointId;
    std::cerr << " " << Mod->getEntryPointName(EntryPointId);
    std::cerr << std::endl;

    // Show current invocation and group.
    const Invocation *Inv = CurrentCommand->getCurrentInvocation();
    const Workgroup *Group = CurrentCommand->getCurrentWorkgroup();
    std::cerr << "    Invocation:";
    std::cerr << " Global" << Inv->getGlobalId();
    std::cerr << " Local" << Inv->getLocalId();
    std::cerr << " Group" << Group->getGroupId();
    std::cerr << std::endl;

    // Show current instruction.
    std::cerr << "      ";
    Inv->getCurrentInstruction()->print(std::cerr, false);
    std::cerr << std::endl;
  }
  else
  {
    std::cerr << "    <origin unknown>";
  }

  std::cerr << std::endl;

  if (CurrentCommand)
    CurrentCommand->signalError();

  if (Fatal)
    abort();
}

#define REPORT(func, ...)                                                      \
  for (auto &P : Plugins)                                                      \
  {                                                                            \
    P.second->func(__VA_ARGS__);                                               \
  }

void Device::reportDispatchCommandBegin(const DispatchCommand *Cmd)
{
  REPORT(dispatchCommandBegin, Cmd);
}

void Device::reportDispatchCommandComplete(const DispatchCommand *Cmd)
{
  REPORT(dispatchCommandComplete, Cmd);
}

void Device::reportInstructionExecuted(const Invocation *Invoc,
                                       const Instruction *Inst)
{
  REPORT(instructionExecuted, Invoc, Inst);
}

void Device::reportInvocationBegin(const Invocation *Invoc)
{
  REPORT(invocationBegin, Invoc);
}

void Device::reportInvocationComplete(const Invocation *Invoc)
{
  REPORT(invocationComplete, Invoc);
}

void Device::reportMemoryLoad(const Memory *Mem, uint64_t Address,
                              uint64_t NumBytes)
{
  if (CurrentCommand && CurrentCommand->isWorkerThread())
  {
    // TODO: Workgroup/subgroup level accesses?
    // TODO: Workgroup/Invocation scope initialization is not covered.
    if (auto *I = CurrentCommand->getCurrentInvocation())
      REPORT(memoryLoad, Mem, Address, NumBytes, I);
  }
  else
  {
    // Must be host accessing device memory.
    assert(Mem->getScope() == MemoryScope::Device);
    REPORT(hostMemoryLoad, Mem, Address, NumBytes);
  }
}

void Device::reportMemoryMap(const Memory *Memory, uint64_t Base,
                             uint64_t Offset, uint64_t NumBytes)
{
  REPORT(memoryMap, Memory, Base, Offset, NumBytes);
}

void Device::reportMemoryStore(const Memory *Mem, uint64_t Address,
                               uint64_t NumBytes, const uint8_t *Data)
{
  if (CurrentCommand && CurrentCommand->isWorkerThread())
  {
    // TODO: Workgroup/subgroup level accesses?
    // TODO: Workgroup/Invocation scope initialization is not covered.
    if (auto *I = CurrentCommand->getCurrentInvocation())
      REPORT(memoryStore, Mem, Address, NumBytes, Data, I);
  }
  else
  {
    // Must be host accessing device memory.
    assert(Mem->getScope() == MemoryScope::Device);
    REPORT(hostMemoryStore, Mem, Address, NumBytes, Data);
  }
}

void Device::reportMemoryUnmap(const Memory *Memory, uint64_t Base)
{
  REPORT(memoryUnmap, Memory, Base);
}

void Device::reportWorkgroupBegin(const Workgroup *Group)
{
  REPORT(workgroupBegin, Group);
}

void Device::reportWorkgroupBarrier(const Workgroup *Group)
{
  REPORT(workgroupBarrier, Group);
}

void Device::reportWorkgroupComplete(const Workgroup *Group)
{
  REPORT(workgroupComplete, Group);
}

#undef REPORT

void Device::run(const DispatchCommand &Command)
{
  // TODO: Mutex instead?
  assert(CurrentCommand == nullptr);

  CurrentCommand = new CommandInvocation(*this, Command);
  CurrentCommand->run();

  delete CurrentCommand;
  CurrentCommand = nullptr;
}

} // namespace talvos
