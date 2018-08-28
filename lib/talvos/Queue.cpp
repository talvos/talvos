// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Queue.cpp
/// This file defines the Queue class.

#include <cassert>

#include "talvos/Queue.h"
#include "talvos/Commands.h"

namespace talvos
{

Queue::Queue(Device &Dev) : Dev(Dev)
{
  Running = true;
  Thread = std::thread(&Queue::run, this);
}

Queue::~Queue()
{
  // Clear commands and signal thread to exit.
  Mutex.lock();
  Running = false;
  while (!Commands.empty())
    Commands.pop();
  StateChanged.notify_all();
  Mutex.unlock();

  Thread.join();
}

void Queue::submit(const std::vector<Command *> &NewCommands, bool *Fence)
{
  std::lock_guard<std::mutex> Lock(Mutex);

  // Add commands to queue.
  for (auto Cmd : NewCommands)
    Commands.push(Cmd);

  if (Fence)
  {
    // Add fence to queue.
    assert(Fences.count(Fence) == 0);
    Fences.insert(Fence);
    Commands.push((Command *)Fence);
  }

  // Signal that queue state has changed.
  StateChanged.notify_all();
}

void Queue::run()
{
  while (Running)
  {
    Command *Cmd = nullptr;

    // Get the next command (waiting for one to arrive if necessary).
    {
      std::unique_lock<std::mutex> Lock(Mutex);

      // If there are no available commands, wait until some are added.
      if (Commands.empty() && Running)
        StateChanged.wait(Lock);

      // If there are still no commands, go round again (probably exiting).
      if (Commands.empty())
        continue;

      // Get the next command.
      Cmd = Commands.front();

      // Check if command is actually a fence.
      if (Fences.count((bool *)Cmd))
      {
        // Signal fence, remove it, and continue.
        *((bool *)Cmd) = true;
        Fences.erase((bool *)Cmd);
        Commands.pop();
        StateChanged.notify_all();
        continue;
      }
    }

    // Run command.
    assert(Cmd);
    Cmd->run(Dev);

    // Remove command from queue.
    Mutex.lock();
    Commands.pop();
    StateChanged.notify_all();
    Mutex.unlock();
  }
}

void Queue::waitIdle()
{
  // Loop until no pending commands.
  while (true)
  {
    std::unique_lock<std::mutex> Lock(Mutex);

    // If queue is already empty, just return.
    if (Commands.empty())
      return;

    // Wait until the queue state changes before trying again.
    StateChanged.wait(Lock);
  }
}

} // namespace talvos
