// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Plugin.h
/// This file declares the Plugin class.

#ifndef TALVOS_PLUGIN_H
#define TALVOS_PLUGIN_H

namespace talvos
{

class DispatchCommand;
class Instruction;
class Invocation;
class Workgroup;

/// Base class for Talvos plugins.
/// Plugins should extend this class and override the callbacks that they are
/// interested in.
class Plugin
{
public:
  virtual ~Plugin() = default;

  /// Called when a dispatch command has begun executing.
  virtual void dispatchCommandBegin(const DispatchCommand *Cmd) {}

  /// Called when a dispatch command has completed.
  virtual void dispatchCommandComplete(const DispatchCommand *Cmd) {}

  /// Called when an instruction has been executed by an invocation.
  virtual void instructionExecuted(const Invocation *Invoc,
                                   const Instruction *Inst)
  {
  }

  /// Called when an invocation has begun executing.
  virtual void invocationBegin(const Invocation *Invoc) {}

  /// Called when an invocation has completed.
  virtual void invocationComplete(const Invocation *Invoc) {}

  /// Called when a workgroup has begun executing.
  virtual void workgroupBegin(const Workgroup *Group) {}

  /// Called when a workgroup barrier is cleared.
  virtual void workgroupBarrier(const Workgroup *Group) {}

  /// Called when a workgroup has completed.
  virtual void workgroupComplete(const Workgroup *Group) {}
};

} // namespace talvos

#endif
