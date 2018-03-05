// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Device.cpp
/// This file defines the Device class.

#include "config.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <sstream>
#include <unistd.h>

#if HAVE_READLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif

#include <spirv/unified1/spirv.h>

#include "Utils.h"
#include "talvos/Device.h"
#include "talvos/DispatchCommand.h"
#include "talvos/Function.h"
#include "talvos/Instruction.h"
#include "talvos/Invocation.h"
#include "talvos/Memory.h"
#include "talvos/Module.h"
#include "talvos/Workgroup.h"

namespace talvos
{

Device::Device()
{
  GlobalMemory = new Memory(*this);

  CurrentCommand = nullptr;
  CurrentInvocation = nullptr;
}

Device::~Device() { delete GlobalMemory; }

void Device::reportError(const std::string &Error)
{
  // TODO: Handle errors that occur outside of command execution.
  assert(CurrentCommand);

  std::cerr << std::endl;
  std::cerr << Error << std::endl;

  // Show current entry point.
  const Module *Mod = CurrentCommand->getModule();
  uint32_t EntryPointId = CurrentCommand->getFunction()->getId();
  std::cerr << "    Entry point:";
  std::cerr << " %" << EntryPointId;
  std::cerr << " " << Mod->getEntryPointName(EntryPointId);
  std::cerr << std::endl;

  // Show current invocation.
  std::cerr << "    Invocation:";
  std::cerr << " Global" << CurrentInvocation->getGlobalId();
  std::cerr << " Local" << CurrentInvocation->getLocalId();
  std::cerr << " Group" << CurrentGroup->getGroupId();
  std::cerr << std::endl;

  // Show current instruction.
  std::cerr << "    ";
  CurrentInvocation->getNextInstruction()->print(std::cerr);
  std::cerr << std::endl;

  std::cerr << std::endl;

  // TODO: Break interactive debugger.
}

void Device::run(const DispatchCommand &Command)
{
  assert(PendingGroups.empty());
  assert(RunningGroups.empty());
  assert(CurrentCommand == nullptr);
  CurrentCommand = &Command;

  Continue = false;
  Interactive = checkEnv("TALVOS_INTERACTIVE", false);
  // TODO: Print info about current command (entry name, dispatch size, etc).

  // Build list of pending group IDs.
  Dim3 NumGroups = Command.getNumGroups();
  for (uint32_t GZ = 0; GZ < NumGroups.Z; GZ++)
    for (uint32_t GY = 0; GY < NumGroups.Y; GY++)
      for (uint32_t GX = 0; GX < NumGroups.X; GX++)
        PendingGroups.push_back({GX, GY, GZ});

  // Loop until all groups are finished.
  // A pool of running groups is maintained to allow the current group to be
  // suspended and changed via the interactive debugger interface.
  NextGroupIndex = 0;
  while (true)
  {
    // Get next group to run.
    // Take from running pool first, then pending pool.
    if (!RunningGroups.empty())
    {
      CurrentGroup = RunningGroups.back();
      RunningGroups.pop_back();
    }
    else if (NextGroupIndex < PendingGroups.size())
    {
      CurrentGroup =
          new Workgroup(*this, Command, PendingGroups[NextGroupIndex]);
      ++NextGroupIndex;
    }
    else
    {
      // All groups are finished.
      break;
    }

    // Loop until all work items in the group have completed.
    while (true)
    {
      // Step each invocation in group until it hits a barrier or completes.
      // Note that the interact() calls can potentially change the current
      // invocation and group being processed.
      while (true)
      {
        // Get the next invocation in the current group in the READY state.
        // TODO: Could move some of this logic into the Workgroup class?
        const Workgroup::WorkItemList &WorkItems = CurrentGroup->getWorkItems();
        auto I =
            std::find_if(WorkItems.begin(), WorkItems.end(), [](const auto &I) {
              return I->getState() == Invocation::READY;
            });
        if (I == WorkItems.end())
          break;
        CurrentInvocation = I->get();

        interact();
        while (CurrentInvocation->getState() == Invocation::READY)
        {
          CurrentInvocation->step();
          interact();
        }
        CurrentInvocation = nullptr;
      }

      // Check for barriers.
      // TODO: Move logic for barrier handling into Workgroup class?
      const Workgroup::WorkItemList &WorkItems = CurrentGroup->getWorkItems();
      uint32_t BarrierCount =
          std::count_if(WorkItems.begin(), WorkItems.end(), [](const auto &I) {
            return I->getState() == Invocation::BARRIER;
          });
      if (BarrierCount > 0)
      {
        // All invocations in the group must hit the barrier.
        // TODO: Ensure they hit the *same* barrier?
        // TODO: Allow for other execution scopes.
        if (BarrierCount != WorkItems.size())
        {
          // TODO: Better error message.
          // TODO: Try to carry on?
          std::cerr << "Barrier not reached by every invocation." << std::endl;
          abort();
        }

        // Clear the barrier.
        for (auto &WI : WorkItems)
          WI->clearBarrier();
      }
      else
      {
        // All invocations must have completed - this group is done.
        delete CurrentGroup;
        CurrentGroup = nullptr;
        break;
      }
    }
  }

  CurrentCommand = nullptr;
  PendingGroups.clear();
}

// Private functions for interactive execution and debugging.

void Device::interact()
{
  if (!Interactive)
    return;

  // Keep going if user used 'continue'.
  if (Continue)
    return;

  printNextInstruction();

  // Loop until the user enters a command that resumes execution.
  bool IsTTY = isatty(STDIN_FILENO);
  while (true)
  {
    // Get line of user input.
    bool eof = false;
    std::string Line;
#if HAVE_READLINE
    if (IsTTY)
    {
      char *CLine = readline("(talvos) ");
      if (CLine)
      {
        Line = CLine;
        free(CLine);
      }
      else
        eof = true;
    }
    else
#endif
    {
      if (IsTTY)
        std::cout << "(talvos) " << std::flush;
      getline(std::cin, Line);
      eof = std::cin.eof();
    }

    // Quit on EOF.
    if (eof)
    {
      if (IsTTY)
        std::cout << "(quit)" << std::endl;
      quit({});
      return;
    }

    // Split line into tokens.
    std::istringstream ISS(Line);
    std::vector<std::string> Tokens{std::istream_iterator<std::string>{ISS},
                                    std::istream_iterator<std::string>{}};

    // Skip empty lines.
    // TODO: Repeat last command instead?
    if (!Tokens.size())
      continue;

#if HAVE_READLINE
    add_history(Line.c_str());
#endif

/// Map a command with a long and short name to a handler function.
#define CMD(LONG, SHORT, FUNC)                                                 \
  if (Tokens[0] == LONG || Tokens[0] == SHORT)                                 \
  {                                                                            \
    if (FUNC(Tokens))                                                          \
      break;                                                                   \
    else                                                                       \
      continue;                                                                \
  }
    CMD("continue", "c", cont);
    CMD("help", "h", help);
    CMD("print", "p", print);
    CMD("quit", "q", quit);
    CMD("step", "s", step);
    CMD("switch", "sw", swtch);
#undef CMD
    std::cerr << "Unrecognized command '" << Tokens[0] << "'" << std::endl;
  }
}

void Device::printNextInstruction()
{
  assert(CurrentInvocation);
  if (CurrentInvocation->getState() == Invocation::BARRIER)
    std::cout << "  <barrier>";
  else if (CurrentInvocation->getState() == Invocation::FINISHED)
    std::cout << "  <finished>";
  else
    CurrentInvocation->getNextInstruction()->print(std::cout);
  std::cout << std::endl;
}

bool Device::cont(const std::vector<std::string> &Args)
{
  Continue = true;
  return true;
}

bool Device::help(const std::vector<std::string> &Args)
{
  std::cout << "Command list:" << std::endl;
  std::cout << "  continue     (c)" << std::endl;
  std::cout << "  help         (h)" << std::endl;
  std::cout << "  print        (p)" << std::endl;
  std::cout << "  quit         (q)" << std::endl;
  std::cout << "  step         (s)" << std::endl;
  std::cout << "  switch       (sw)" << std::endl;
  // TODO: help for specific commands
  // std::cout << "(type 'help <command>' for more information)" << std::endl;

  return false;
}

bool Device::print(const std::vector<std::string> &Args)
{
  if (Args.size() != 2)
  {
    std::cerr << "Usage: print %<id>" << std::endl;
    return false;
  }

  // Parse result ID.
  char *Next;
  uint32_t Id = strtoul(Args[1].c_str() + 1, &Next, 10);
  if (Args[1][0] != '%' || strlen(Next))
  {
    std::cerr << "Invalid result ID" << std::endl;
    return false;
  }

  std::cout << "  %" << std::dec << Id << " = ";

  // Handle types.
  if (const Type *Ty = CurrentCommand->getModule()->getType(Id))
  {
    std::cout << Ty << std::endl;
    return false;
  }

  // Print object value for current invocation.
  std::cout << CurrentInvocation->getObject(Id) << std::endl;

  return false;
}

bool Device::quit(const std::vector<std::string> &Args) { exit(0); }

bool Device::step(const std::vector<std::string> &Args)
{
  if (CurrentInvocation->getState() == Invocation::FINISHED)
  {
    std::cout << "Invocation has finished." << std::endl;
    return false;
  }
  else if (CurrentInvocation->getState() == Invocation::BARRIER)
  {
    std::cout << "Invocation is at a barrier." << std::endl;
    return false;
  }

  return true;
}

bool Device::swtch(const std::vector<std::string> &Args)
{
  // TODO: Allow `select group X Y Z` or `select local X Y Z` as well?
  if (Args.size() < 2 || Args.size() > 4)
  {
    std::cerr << "Usage: switch X [Y [Z]]" << std::endl;
    return false;
  }

  // Parse global invocation ID.
  Dim3 Id(0, 0, 0);
  for (unsigned i = 1; i < Args.size(); i++)
  {
    char *Next;
    Id[i - 1] = strtoul(Args[i].c_str(), &Next, 10);
    if (strlen(Next))
    {
      std::cerr << "Invalid global ID '" << Args[i] << "'" << std::endl;
      return false;
    }
  }

  // Check global index is within global bounds.
  Dim3 GroupSize = CurrentCommand->getGroupSize();
  Dim3 NumGroups = CurrentCommand->getNumGroups();
  if (Id.X >= GroupSize.X * NumGroups.X || Id.Y >= GroupSize.Y * NumGroups.Y ||
      Id.Z >= GroupSize.Z * NumGroups.Z)
  {
    std::cerr << "Global ID is out of the bounds of the current dispatch."
              << std::endl;
    return false;
  }

  // Check if we are already executing the target invocation.
  if (CurrentInvocation->getGlobalId() == Id)
  {
    std::cerr << "Already executing this invocation!" << std::endl;
    return false;
  }

  // Find workgroup with target group ID.
  Dim3 GroupId(Id.X / GroupSize.X, Id.Y / GroupSize.Y, Id.Z / GroupSize.Z);
  Workgroup *Group = nullptr;
  if (GroupId == CurrentGroup->getGroupId())
  {
    // Already running - nothing to do.
    Group = CurrentGroup;
  }
  if (!Group)
  {
    // Check running groups list.
    auto RG = std::find_if(
        RunningGroups.begin(), RunningGroups.end(),
        [&GroupId](const Workgroup *G) { return G->getGroupId() == GroupId; });
    if (RG != RunningGroups.end())
    {
      // Remove from running groups.
      RunningGroups.erase(RG);
      Group = *RG;
    }
  }
  if (!Group)
  {
    // Check pending groups list.
    auto PG = std::find(PendingGroups.begin() + NextGroupIndex,
                        PendingGroups.end(), GroupId);
    if (PG != PendingGroups.end())
    {
      // Remove from pending groups and create the new workgroup.
      Group = new Workgroup(*this, *CurrentCommand, *PG);
      PendingGroups.erase(PG);
    }
  }

  if (!Group)
  {
    std::cerr << "Workgroup containing invocation has already finished."
              << std::endl;
    return false;
  }

  // Switch to target group.
  if (Group != CurrentGroup)
  {
    RunningGroups.push_back(CurrentGroup);
    CurrentGroup = Group;
  }

  // Switch to target invocation.
  Dim3 LocalId(Id.X % GroupSize.X, Id.Y % GroupSize.Y, Id.Z % GroupSize.Z);
  uint32_t LocalIndex =
      LocalId.X + (LocalId.Y + LocalId.Z * GroupSize.Y) * GroupSize.X;
  CurrentInvocation = CurrentGroup->getWorkItems()[LocalIndex].get();

  std::cout << "Switched to invocation with global ID " << Id << std::endl;

  printNextInstruction();

  return false;
}

} // namespace talvos
