// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Queue.h
/// This file declares the Queue class.

#ifndef TALVOS_QUEUE_H
#define TALVOS_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>
#include <set>
#include <thread>

namespace talvos
{

class Command;
class Device;

/// This class represents a queue for executing commands on a device.
class Queue
{
public:
  /// Create a queue for the specified device.
  Queue(Device &Dev);

  /// Destroy the queue.
  ~Queue();

  // Do not allow Queue objects to be copied.
  ///\{
  Queue(const Queue &) = delete;
  Queue &operator=(const Queue &) = delete;
  ///\}

  /// Submit a batch of commands to the queue.
  /// If \p Fence is not `nullptr`, its pointee will be set to `true` when all
  /// of the commands have completed.
  void submit(const std::vector<Command *> &NewCommands, bool *Fence = nullptr);

  /// Wait until all commands in the queue have completed.
  void waitIdle();

private:
  /// The device that this queue will execute work on.
  Device &Dev;

  /// The queue of pending commands.
  std::queue<Command *> Commands;

  /// A set of pending fences.
  std::set<bool *> Fences;

  // Background queue thread used to progress command execution.
  std::thread Thread;

  /// Mutex used to guard queue updates.
  std::mutex Mutex;

  /// Condition variable to signal when queue state has changed.
  std::condition_variable StateChanged;

  /// Flag to signal whether the queue thread is active.
  bool Running;

  /// Entry point for background queue thread.
  void run();
};

} // namespace talvos

#endif
