// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file PipelineExecutor.cpp
/// This file defines the PipelineExecutor class.

#include "config.h"

#include <algorithm>
#include <cassert>
#include <cmath>
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
#include "talvos/EntryPoint.h"
#include "talvos/GraphicsPipeline.h"
#include "talvos/Instruction.h"
#include "talvos/Invocation.h"
#include "talvos/Memory.h"
#include "talvos/Module.h"
#include "talvos/PipelineStage.h"
#include "talvos/RenderPass.h"
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

/// State to be carried through the execution of a render pipeline.
struct PipelineExecutor::RenderPipelineState
{
  /// The outputs from the vertex shading stage.
  std::vector<VertexOutput> VertexOutputs;
};

/// Outputs from a vertex shading stage.
struct PipelineExecutor::VertexOutput
{
  std::map<SpvBuiltIn, Object> BuiltIns; ///< BuiltIn variables.
  std::map<uint32_t, Object> Locations;  ///< Location variables.
};

// TODO: Define proper Vec2/Vec3/Vec3/Vec<N> classes?
struct Vec2
{
  float X, Y;
};

PipelineExecutor::PipelineExecutor(PipelineExecutorKey Key, Device &Dev)
    : Dev(Dev), CurrentCommand(nullptr), CurrentStage(nullptr)
{}

Workgroup *PipelineExecutor::createWorkgroup(Dim3 GroupId) const
{
  const DispatchCommand *DC = (const DispatchCommand *)CurrentCommand;

  // Create workgroup.
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
        for (auto Var : CurrentStage->getEntryPoint()->getVariables())
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
            PipelineMemory->store(Address, Sz,
                                  (uint8_t *)DC->getNumGroups().Data);
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
  assert(CurrentCommand == nullptr);
  CurrentCommand = &Cmd;

  CurrentStage = Cmd.getPipeline()->getStage();

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
  for (uint32_t GZ = 0; GZ < Cmd.getNumGroups().Z; GZ++)
    for (uint32_t GY = 0; GY < Cmd.getNumGroups().Y; GY++)
      for (uint32_t GX = 0; GX < Cmd.getNumGroups().X; GX++)
        PendingGroups.push_back({GX, GY, GZ});

  NextWorkIndex = 0;

  // Create worker threads.
  NumThreads = 1;
  if (!Interactive && Dev.isThreadSafe())
    NumThreads = (uint32_t)getEnvUInt("TALVOS_NUM_WORKERS",
                                      std::thread::hardware_concurrency());
  std::vector<std::thread> Threads;
  for (unsigned i = 0; i < NumThreads; i++)
    Threads.push_back(std::thread(&PipelineExecutor::runComputeWorker, this));

  // Wait for workers to complete
  for (unsigned i = 0; i < NumThreads; i++)
    Threads[i].join();

  PendingGroups.clear();
  CurrentCommand = nullptr;
}

void PipelineExecutor::run(const talvos::DrawCommand &Cmd)
{
  assert(CurrentCommand == nullptr);
  CurrentCommand = &Cmd;

  CurrentStage = Cmd.getPipeline()->getVertexStage();

  Objects = CurrentStage->getObjects();
  // TODO: Handle DescriptorSetMap

  Continue = false;
  Interactive = checkEnv("TALVOS_INTERACTIVE", false);

  NumThreads = 1;
  if (!Interactive && Dev.isThreadSafe())
    NumThreads = (uint32_t)getEnvUInt("TALVOS_NUM_WORKERS",
                                      std::thread::hardware_concurrency());

  // Set up vertex shader stage pipeline memories.
  RenderPipelineState State;
  State.VertexOutputs.resize(Cmd.getNumVertices());

  // Create worker threads for vertex shader.
  NextWorkIndex = 0;
  std::vector<std::thread> Threads;
  for (unsigned i = 0; i < NumThreads; i++)
    Threads.push_back(
        std::thread(&PipelineExecutor::runVertexWorker, this, &State));

  // Wait for vertex shader workers to complete.
  for (unsigned i = 0; i < NumThreads; i++)
    Threads[i].join();

  // Switch to fragment shader for rasterization.
  CurrentStage = Cmd.getPipeline()->getFragmentStage();
  assert(CurrentStage && "rendering without fragment shader not implemented");
  Objects = CurrentStage->getObjects();

  const RenderPassInstance &RPI = Cmd.getRenderPassInstance();
  const RenderPass &RP = RPI.getRenderPass();
  const Framebuffer &FB = RPI.getFramebuffer();

  // TODO: Handle instancing
  assert(Cmd.getNumInstances() == 1);

  // TODO: Handle other topologies
  VkPrimitiveTopology Topology = Cmd.getPipeline()->getTopology();
  switch (Topology)
  {
  case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
  {
    for (uint32_t v = 0; v < Cmd.getNumVertices(); v += 3)
      rasterizeTriangle(RP, FB, State.VertexOutputs[v],
                        State.VertexOutputs[v + 1], State.VertexOutputs[v + 2]);
    break;
  }
  case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
  {
    for (uint32_t v = 2; v < Cmd.getNumVertices(); v++)
    {
      const VertexOutput &A = State.VertexOutputs[v - 2];
      const VertexOutput &B = State.VertexOutputs[v - 1];

      const VertexOutput &C = State.VertexOutputs[v];
      rasterizeTriangle(RP, FB, A, B, C);

      if (++v >= Cmd.getNumVertices())
        break;

      const VertexOutput &D = State.VertexOutputs[v];
      rasterizeTriangle(RP, FB, B, D, C);
    }
    break;
  }
  default:
    std::cerr << "Unimplemented primitive topology: " << Topology << std::endl;
    abort();
  }

  CurrentStage = nullptr;
  CurrentCommand = nullptr;
}

void PipelineExecutor::runComputeWorker()
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
    else if (NextWorkIndex < PendingGroups.size())
    {
      size_t GroupIndex = NextWorkIndex++;
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

void PipelineExecutor::runVertexWorker(struct RenderPipelineState *State)
{
  IsWorkerThread = true;
  CurrentInvocation = nullptr;

  const DrawCommand *DC = (const DrawCommand *)CurrentCommand;
  const GraphicsPipeline *Pipeline = DC->getPipeline();

  // Loop until all vertices are finished.
  while (true)
  {
    // Get next vertex index.
    uint32_t VertexIndex = (uint32_t)NextWorkIndex++;
    if (VertexIndex >= DC->getNumVertices())
      break;

    std::vector<Object> InitialObjects = Objects;

    // Create pipeline memory and populate with input/output variables.
    std::shared_ptr<Memory> PipelineMemory =
        std::make_shared<Memory>(Dev, MemoryScope::Invocation);
    std::map<const Variable *, uint64_t> OutputAddresses;
    for (auto Var : CurrentStage->getEntryPoint()->getVariables())
    {
      const Type *Ty = Var->getType();
      if (Ty->getStorageClass() == SpvStorageClassInput)
      {
        // Allocate storage for input variable.
        size_t ElemSize = Ty->getElementType()->getSize();
        uint64_t Address = PipelineMemory->allocate(ElemSize);
        InitialObjects[Var->getId()] = Object(Ty, Address);

        // Initialize input variable data.
        if (Var->hasDecoration(SpvDecorationLocation))
        {
          uint32_t Location = Var->getDecoration(SpvDecorationLocation);

          // Get vertex attribute description.
          auto &Attributes = Pipeline->getVertexAttributeDescriptions();
          auto Attr = std::find_if(
              Attributes.begin(), Attributes.end(),
              [Location](auto Elem) { return Elem.location == Location; });
          assert(Attr != Attributes.end() && "invalid attribute location");

          // Get vertex binding description.
          auto &Bindings = Pipeline->getVertexBindingDescriptions();
          auto Binding =
              std::find_if(Bindings.begin(), Bindings.end(), [Attr](auto Elem) {
                return Elem.binding == Attr->binding;
              });
          assert(Binding != Bindings.end() && "invalid binding number");

          // TODO: Handle VK_VERTEX_INPUT_RATE_INSTANCE
          assert(Binding->inputRate == VK_VERTEX_INPUT_RATE_VERTEX);

          // TODO: Handle other formats
          assert(Attr->format == VK_FORMAT_R32G32B32A32_SFLOAT);

          // Calculate variable address in vertex buffer memory.
          uint64_t ElemAddr = DC->getVertexBindings().at(Attr->binding);
          ElemAddr += VertexIndex * Binding->stride;
          ElemAddr += Attr->offset;

          // Copy variable data to pipeline memory.
          Memory::copy(Address, *PipelineMemory, ElemAddr,
                       Dev.getGlobalMemory(), ElemSize);
        }
        else
        {
          assert(false && "Unhandled input variable type");
        }
      }
      else if (Ty->getStorageClass() == SpvStorageClassOutput)
      {
        // Allocate storage for output variable and store address.
        uint64_t Address =
            PipelineMemory->allocate(Ty->getElementType()->getSize());
        InitialObjects[Var->getId()] = Object(Ty, Address);
        OutputAddresses[Var] = Address;
      }
    }

    // Create shader invocation.
    CurrentInvocation =
        new Invocation(Dev, *CurrentStage, InitialObjects, PipelineMemory,
                       nullptr, Dim3(VertexIndex, 0, 0));

    // Run shader invocation to completion.
    interact();
    while (CurrentInvocation->getState() == Invocation::READY)
    {
      CurrentInvocation->step();
      interact();
    }

    delete CurrentInvocation;
    CurrentInvocation = nullptr;

    // Gather output variables.
    for (auto Var : CurrentStage->getModule()->getVariables())
    {
      if (Var->getType()->getStorageClass() != SpvStorageClassOutput)
        continue;

      uint64_t BaseAddress = OutputAddresses[Var];
      const Type *Ty = Var->getType()->getElementType();

      if (Var->hasDecoration(SpvDecorationBuiltIn))
      {
        SpvBuiltIn BuiltIn =
            (SpvBuiltIn)Var->getDecoration(SpvDecorationBuiltIn);
        State->VertexOutputs[VertexIndex].BuiltIns[BuiltIn] =
            Object::load(Ty, *PipelineMemory, BaseAddress);
      }
      else if (Var->hasDecoration(SpvDecorationLocation))
      {
        // TODO: Handle output variables decorated with Location.
        std::cerr << "Unimplemented: Output variable with Location decoration"
                  << std::endl;
        abort();
      }
      else if (Ty->getTypeId() == Type::STRUCT &&
               Ty->getStructMemberDecorations(0).count(SpvDecorationBuiltIn))
      {
        // Load builtin from each structure member.
        for (uint32_t i = 0; i < Ty->getElementCount(); i++)
        {
          uint64_t Address = BaseAddress + Ty->getElementOffset(i);
          SpvBuiltIn BuiltIn = (SpvBuiltIn)Ty->getStructMemberDecorations(i).at(
              SpvDecorationBuiltIn);
          State->VertexOutputs[VertexIndex].BuiltIns[BuiltIn] =
              Object::load(Ty->getElementType(i), *PipelineMemory, Address);
        }
      }
      else
      {
        assert(false && "Unhandled output variable type");
      }
    }
  }
}

void PipelineExecutor::rasterizeTriangle(const RenderPass &RP,
                                         const Framebuffer &FB,
                                         const VertexOutput &VA,
                                         const VertexOutput &VB,
                                         const VertexOutput &VC)
{
  // TODO: Parallelize rasterization?
  IsWorkerThread = true;

  // Gather vertex positions for the primitive.
  Vec2 A, B, C;
  auto getPosition = [](const VertexOutput &V, Vec2 &Pos) {
    assert(V.BuiltIns.count(SpvBuiltInPosition));

    const Object &Position = V.BuiltIns.at(SpvBuiltInPosition);
    assert(Position.getType()->isVector() &&
           Position.getType()->getElementType()->isFloat() &&
           Position.getType()->getElementType()->getBitWidth() == 32 &&
           "Position built-in type must be float4");

    memcpy(&Pos, Position.getData(), sizeof(Vec2));
  };
  getPosition(VA, A);
  getPosition(VB, B);
  getPosition(VC, C);

  // Compute axis-aligned bounding box.
  float XMin = std::fmin(A.X, std::fmin(B.X, C.X));
  float YMin = std::fmin(A.Y, std::fmin(B.Y, C.Y));
  float XMax = std::fmax(A.X, std::fmax(B.X, C.X));
  float YMax = std::fmax(A.Y, std::fmax(B.Y, C.Y));

  // Compute pixel increment values in normalized device coordinates.
  float XInc = 2.f / FB.getWidth();
  float YInc = 2.f / FB.getHeight();

  // Loop over pixels in axis-aligned bounding box.
  for (float y = YMin; y < YMax; y += YInc)
  {
    for (float x = XMin; x < XMax; x += XInc)
    {
      // Compute barycentric coordinates.
      float Div = (B.Y - C.Y) * (A.X - C.X) + (C.X - B.X) * (A.Y - C.Y);
      float a = (((B.Y - C.Y) * (x - C.X)) + ((C.X - B.X) * (y - C.Y))) / Div;
      float b = (((C.Y - A.Y) * (x - C.X)) + ((A.X - C.X) * (y - C.Y))) / Div;
      float c = 1.f - a - b;

      // Check if pixel is inside triangle.
      if (a >= 0 && b >= 0 && c >= 0)
      {
        struct FragmentOutput
        {
          uint64_t Address;
          uint32_t Location;
          uint32_t Component;
        };

        // Create pipeline memory and populate with input/output variables.
        std::vector<Object> InitialObjects = Objects;
        std::shared_ptr<Memory> PipelineMemory =
            std::make_shared<Memory>(Dev, MemoryScope::Invocation);
        std::map<const Variable *, FragmentOutput> Outputs;
        for (auto Var : CurrentStage->getEntryPoint()->getVariables())
        {
          const Type *Ty = Var->getType();
          if (Ty->getStorageClass() == SpvStorageClassInput)
          {
            assert(false && "fragment shader input variables not implemented");
          }
          else if (Ty->getStorageClass() == SpvStorageClassOutput)
          {
            // Allocate storage for output variable.
            uint64_t Address =
                PipelineMemory->allocate(Ty->getElementType()->getSize());
            InitialObjects[Var->getId()] = Object(Ty, Address);

            // Store output variable information.
            assert(Var->hasDecoration(SpvDecorationLocation));
            uint32_t Location = Var->getDecoration(SpvDecorationLocation);
            uint32_t Component = 0;
            if (Var->hasDecoration(SpvDecorationComponent))
              Var->getDecoration(SpvDecorationComponent);
            Outputs[Var] = {Address, Location, Component};
          }
        }

        // Create fragment shader invocation.
        CurrentInvocation =
            new Invocation(Dev, *CurrentStage, InitialObjects, PipelineMemory,
                           nullptr, Dim3(0, 0, 0));

        // Run shader invocation to completion.
        interact();
        while (CurrentInvocation->getState() == Invocation::READY)
        {
          CurrentInvocation->step();
          interact();
        }

        delete CurrentInvocation;
        CurrentInvocation = nullptr;

        // Convert pixel coordinates to framebuffer space.
        int XF = std::round((FB.getWidth() / 2) * x + (FB.getWidth() / 2));
        int YF = std::round((FB.getHeight() / 2) * y + (FB.getHeight() / 2));

        // Write fragment outputs to color attachments.
        std::vector<uint32_t> ColorAttachments =
            RP.getSubpass(0).ColorAttachments;
        for (auto Output : Outputs)
        {
          uint32_t Location = Output.second.Location;
          assert(Location < ColorAttachments.size());

          uint32_t Ref = ColorAttachments[Location];
          assert(Ref < RP.getNumAttachments());
          assert(Ref < FB.getAttachments().size());

          // Get output variable data.
          // TODO: Handle other formats
          assert(RP.getAttachment(Ref).format == VK_FORMAT_R8G8B8A8_UNORM);
          const Object &OutputData =
              Object::load(Output.first->getType()->getElementType(),
                           *PipelineMemory, Output.second.Address);
          auto convert = [](float v) -> uint8_t {
            if (v < 0.f)
              return 0;
            else if (v >= 1.f)
              return 255;
            else
              return std::round(v * 255);
          };
          uint8_t Pixel[4] = {convert(OutputData.get<float>(0)),
                              convert(OutputData.get<float>(1)),
                              convert(OutputData.get<float>(2)),
                              convert(OutputData.get<float>(3))};

          // Write pixel color to attachment.
          uint64_t Address = FB.getAttachments()[Ref];
          Address += (XF + YF * FB.getWidth()) * 4;
          Dev.getGlobalMemory().store(Address, 4, Pixel);
        }
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
  // TODO: Implement switch for vertex/fragment shaders.
  if (CurrentCommand->getType() != Command::DISPATCH)
  {
    std::cerr << "switch not implemented for this command." << std::endl;
    return false;
  }

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
  Dim3 NumGroups = ((const DispatchCommand *)CurrentCommand)->getNumGroups();
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
    auto PG = std::find(PendingGroups.begin() + NextWorkIndex,
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
