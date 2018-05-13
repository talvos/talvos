// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file PipelineExecutor.cpp
/// This file defines the PipelineExecutor class.

#include "config.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <sstream>
#include <thread>

#if defined(_WIN32) && !defined(__MINGW32__)
#include <io.h>
#include <windows.h>
#define isatty _isatty
#define STDIN_FILENO _fileno(stdin)
#undef ERROR
#undef VOID
#else
#include <unistd.h>
#endif

#if HAVE_READLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif

#include <spirv/unified1/spirv.h>

#include "PipelineExecutor.h"
#include "Utils.h"
#include "talvos/Commands.h"
#include "talvos/ComputePipeline.h"
#include "talvos/Device.h"
#include "talvos/Instruction.h"
#include "talvos/Invocation.h"
#include "talvos/Memory.h"
#include "talvos/Module.h"
#include "talvos/PipelineStage.h"
#include "talvos/Variable.h"
#include "talvos/Workgroup.h"

/// The number of lines before and after the current instruction to print.
#define CONTEXT_SIZE 3

namespace talvos
{

/// Variables for worker thread state.
static thread_local bool IsWorkerThread = false;
static thread_local Workgroup *CurrentGroup;
static thread_local Invocation *CurrentInvocation;

uint32_t PipelineExecutor::NextBreakpoint = 1;
std::map<uint32_t, uint32_t> PipelineExecutor::Breakpoints;

PipelineExecutor::PipelineExecutor(Device &Dev)
    : Dev(Dev), CurrentStage(nullptr)
{}

Workgroup *PipelineExecutor::createWorkgroup(Dim3 GroupId) const
{
  Workgroup *Group = new Workgroup(Dev, *this, GroupId);

  // Create invocations for this group.
  Dim3 GroupSize = CurrentStage->getGroupSize();
  for (uint32_t LZ = 0; LZ < GroupSize.Z; LZ++)
  {
    for (uint32_t LY = 0; LY < GroupSize.Y; LY++)
    {
      for (uint32_t LX = 0; LX < GroupSize.X; LX++)
      {
        Dim3 LocalId(LX, LY, LZ);
        Dim3 GlobalId = LocalId + GroupId * GroupSize;
        std::vector<Object> InitialObjects = Objects;

        // Create pipeline memory and populate with builtin variables.
        std::shared_ptr<Memory> PipelineMemory =
            std::make_shared<Memory>(Dev, MemoryScope::Invocation);
        for (auto Var : CurrentStage->getModule()->getVariables())
        {
          const Type *Ty = Var->getType();
          if (Ty->getStorageClass() != SpvStorageClassInput)
            continue;

          size_t Sz = Ty->getElementType()->getSize();
          uint64_t Address = PipelineMemory->allocate(Sz);
          switch (Var->getDecoration(SpvDecorationBuiltIn))
          {
          case SpvBuiltInGlobalInvocationId:
            PipelineMemory->store(Address, Sz, (uint8_t *)GlobalId.Data);
            break;
          case SpvBuiltInLocalInvocationId:
            PipelineMemory->store(Address, Sz, (uint8_t *)LocalId.Data);
            break;
          case SpvBuiltInNumWorkgroups:
            PipelineMemory->store(Address, Sz, (uint8_t *)NumGroups.Data);
            break;
          case SpvBuiltInWorkgroupId:
            PipelineMemory->store(Address, Sz, (uint8_t *)GroupId.Data);
            break;
          default:
            std::cerr << "Unimplemented input variable builtin: "
                      << Var->getDecoration(SpvDecorationBuiltIn) << std::endl;
            abort();
          }

          // Set pointer value.
          InitialObjects[Var->getId()] = Object(Ty, Address);
        }

        // Create invocation and add to group.
        Group->addWorkItem(
            std::make_unique<Invocation>(Dev, *CurrentStage, InitialObjects,
                                         PipelineMemory, Group, GlobalId));
      }
    }
  }

  return Group;
}

const Invocation *PipelineExecutor::getCurrentInvocation() const
{
  return CurrentInvocation;
}

const Workgroup *PipelineExecutor::getCurrentWorkgroup() const
{
  return CurrentGroup;
}

bool PipelineExecutor::isWorkerThread() const { return IsWorkerThread; }

void PipelineExecutor::run(const talvos::DispatchCommand &Cmd)
{
  assert(CurrentStage == nullptr);

  CurrentStage = Cmd.getPipeline()->getStage();
  NumGroups = Cmd.getNumGroups();

  Objects = CurrentStage->getObjects();

  // Resolve buffer variables.
  const DescriptorSetMap &DSM = Cmd.getDescriptorSetMap();
  for (auto V : CurrentStage->getModule()->getVariables())
  {
    if (!V->isBufferVariable())
      continue;

    // Look up variable in descriptor set and set pointer value if present.
    uint32_t Set = V->getDecoration(SpvDecorationDescriptorSet);
    uint32_t Binding = V->getDecoration(SpvDecorationBinding);
    if (!DSM.count(Set))
      continue;
    if (!DSM.at(Set).count(Binding))
      continue;
    Objects[V->getId()] = Object(V->getType(), DSM.at(Set).at(Binding));
  }

  assert(PendingGroups.empty());
  assert(RunningGroups.empty());

  Continue = false;
  Interactive = checkEnv("TALVOS_INTERACTIVE", false);
  // TODO: Print info about current command (entry name, dispatch size, etc).

  // Build list of pending group IDs.
  for (uint32_t GZ = 0; GZ < NumGroups.Z; GZ++)
    for (uint32_t GY = 0; GY < NumGroups.Y; GY++)
      for (uint32_t GX = 0; GX < NumGroups.X; GX++)
        PendingGroups.push_back({GX, GY, GZ});

  NextGroupIndex = 0;

  // Create worker threads.
  NumThreads = 1;
  if (!Interactive && Dev.isThreadSafe())
    NumThreads = (uint32_t)getEnvUInt("TALVOS_NUM_WORKERS",
                                      std::thread::hardware_concurrency());
  std::vector<std::thread> Threads;
  for (unsigned i = 0; i < NumThreads; i++)
    Threads.push_back(std::thread(&PipelineExecutor::runWorker, this));

  // Wait for workers to complete
  for (unsigned i = 0; i < NumThreads; i++)
    Threads[i].join();

  PendingGroups.clear();
}

void PipelineExecutor::runWorker()
{
  IsWorkerThread = true;
  CurrentInvocation = nullptr;

  // Loop until all groups are finished.
  // A pool of running groups is maintained to allow the current group to be
  // suspended and changed via the interactive debugger interface.
  while (true)
  {
    // Get next group to run.
    // Take from running pool first, then pending pool.
    if (!RunningGroups.empty())
    {
      assert(NumThreads == 1);
      CurrentGroup = RunningGroups.back();
      RunningGroups.pop_back();
    }
    else if (NextGroupIndex < PendingGroups.size())
    {
      size_t GroupIndex = NextGroupIndex++;
      if (GroupIndex >= PendingGroups.size())
        break;
      CurrentGroup = createWorkgroup(PendingGroups[GroupIndex]);
      Dev.reportWorkgroupBegin(CurrentGroup);
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
      size_t BarrierCount =
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
        Dev.reportWorkgroupBarrier(CurrentGroup);
      }
      else
      {
        // All invocations must have completed - this group is done.
        Dev.reportWorkgroupComplete(CurrentGroup);
        delete CurrentGroup;
        CurrentGroup = nullptr;
        break;
      }
    }
  }
}

void PipelineExecutor::signalError()
{
  // Drop to interactive prompt.
  Continue = false;
  interact();
}

// Private functions for interactive execution and debugging.

void PipelineExecutor::interact()
{
  if (!Interactive)
    return;

  // Check if a breakpoint has been reached.
  const Instruction *CI = CurrentInvocation->getCurrentInstruction();
  if (CI && CI->getResultType() &&
      CurrentInvocation->getState() != Invocation::BARRIER)
  {
    uint32_t ResultId = CI->getOperand(1);
    auto BP = std::find_if(
        Breakpoints.begin(), Breakpoints.end(),
        [ResultId](const auto &BP) { return BP.second == ResultId; });
    if (BP != Breakpoints.end())
    {
      std::cout << "Breakpoint " << BP->first << " hit by invocation "
                << CurrentInvocation->getGlobalId() << std::endl;
      Continue = false;
    }
  }

  // Keep going if user used 'continue'.
  if (Continue)
    return;

  printContext();

  // Loop until the user enters a command that resumes execution.
  bool IsTTY = isatty(STDIN_FILENO) == 1;
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
    if (!Tokens.size())
    {
      // Repeat last command if possible, otherwise skip.
      if (!LastLine.size())
        continue;
      Tokens = LastLine;
    }
    else
    {
      // Save tokens for repeating command.
      LastLine = Tokens;
#if HAVE_READLINE
      add_history(Line.c_str());
#endif
    }

/// Map a command with a long and short name to a handler function.
#define CMD(LONG, SHORT, FUNC)                                                 \
  if (Tokens[0] == LONG || Tokens[0] == SHORT)                                 \
  {                                                                            \
    if (FUNC(Tokens))                                                          \
      break;                                                                   \
    else                                                                       \
      continue;                                                                \
  }
    CMD("break", "b", brk);
    CMD("breakpoint", "bp", breakpoint);
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

void PipelineExecutor::printContext() const
{
  assert(CurrentInvocation);
  if (CurrentInvocation->getState() == Invocation::FINISHED)
    std::cout << "  <finished>" << std::endl;
  else
  {
    const Instruction *CI = CurrentInvocation->getCurrentInstruction();
    const Instruction *I = CI;

    // Print set of instructions around current location.
    // TODO: Show instructions in adjacent blocks?
    int i;
    for (i = 0; i > -CONTEXT_SIZE; i--)
    {
      if (!I->previous())
        break;
      I = I->previous();
    }
    for (; i < CONTEXT_SIZE + 1; i++)
    {
      if (CI == I)
      {
        std::cout << "-> ";
        if (CurrentInvocation->getState() == Invocation::BARRIER)
          std::cout << "  <barrier>" << std::endl << "   ";
      }
      else
        std::cout << "   ";

      I->print(std::cout);
      std::cout << std::endl;

      I = I->next();
      if (!I)
        break;
    }
  }
}

bool PipelineExecutor::brk(const std::vector<std::string> &Args)
{
  if (Args.size() != 2)
  {
    std::cerr << "Usage: break %id" << std::endl;
    return false;
  }

  // Parse target result ID.
  char *Next;
  uint32_t Id = (uint32_t)strtoul(Args[1].c_str() + 1, &Next, 10);
  if (Args[1][0] != '%' || strlen(Next))
  {
    std::cerr << "Invalid result ID '" << Args[1] << "'" << std::endl;
    return false;
  }

  // Set breakpoint.
  Breakpoints[NextBreakpoint] = Id;
  std::cout << "Breakpoint " << NextBreakpoint << " set for result ID %" << Id
            << std::endl;
  NextBreakpoint++;

  return false;
}

bool PipelineExecutor::breakpoint(const std::vector<std::string> &Args)
{
  if (Args.size() < 2)
  {
    std::cerr << "Usage: breakpoint [clear|delete|list]" << std::endl;
    return false;
  }

  if (Args[1] == "clear")
  {
    Breakpoints.clear();
    std::cout << "All breakpoints cleared." << std::endl;
  }
  else if (Args[1] == "delete")
  {
    if (Args.size() != 3)
    {
      std::cerr << "Usage: breakpoint delete ID" << std::endl;
      return false;
    }

    // Parse breakpoint ID.
    char *Next;
    uint32_t Id = (uint32_t)strtoul(Args[2].c_str(), &Next, 10);
    if (strlen(Next) || !Breakpoints.count(Id))
    {
      std::cerr << "Invalid breakpoint ID '" << Args[2] << "'" << std::endl;
      return false;
    }

    Breakpoints.erase(Id);
    std::cout << "Breakpoint " << Id << " deleted." << std::endl;
  }
  else if (Args[1] == "list")
  {
    if (Breakpoints.empty())
      std::cout << "No breakpoints." << std::endl;
    else
    {
      for (auto &BP : Breakpoints)
        std::cout << "Breakpoint " << BP.first << ": %" << BP.second
                  << std::endl;
    }
  }
  else
    std::cerr << "Usage: breakpoint [clear|delete|list]" << std::endl;

  return false;
}

bool PipelineExecutor::cont(const std::vector<std::string> &Args)
{
  Continue = true;
  return true;
}

bool PipelineExecutor::help(const std::vector<std::string> &Args)
{
  std::cout << "Command list:" << std::endl;
  std::cout << "  break        (b)" << std::endl;
  std::cout << "  breakpoint   (bp)" << std::endl;
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

bool PipelineExecutor::print(const std::vector<std::string> &Args)
{
  if (Args.size() != 2)
  {
    std::cerr << "Usage: print %<id>" << std::endl;
    return false;
  }

  // Parse result ID.
  char *Next;
  uint32_t Id = (uint32_t)strtoul(Args[1].c_str() + 1, &Next, 10);
  if (Args[1][0] != '%' || strlen(Next))
  {
    std::cerr << "Invalid result ID" << std::endl;
    return false;
  }

  std::cout << "  %" << std::dec << Id << " = ";

  // Handle types.
  if (const Type *Ty = CurrentStage->getModule()->getType(Id))
  {
    std::cout << Ty << std::endl;
    return false;
  }

  // Print object value for current invocation.
  std::cout << CurrentInvocation->getObject(Id) << std::endl;

  return false;
}

bool PipelineExecutor::quit(const std::vector<std::string> &Args) { exit(0); }

bool PipelineExecutor::step(const std::vector<std::string> &Args)
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

bool PipelineExecutor::swtch(const std::vector<std::string> &Args)
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
    Id[i - 1] = (uint32_t)strtoul(Args[i].c_str(), &Next, 10);
    if (strlen(Next))
    {
      std::cerr << "Invalid global ID '" << Args[i] << "'" << std::endl;
      return false;
    }
  }

  // Check global index is within global bounds.
  Dim3 GroupSize = CurrentStage->getGroupSize();
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
      Group = *RG;
      RunningGroups.erase(RG);
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
      Group = createWorkgroup(*PG);
      Dev.reportWorkgroupBegin(Group);
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

  printContext();

  return false;
}

} // namespace talvos
