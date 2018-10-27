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
#define NOMINMAX
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
#include "talvos/Image.h"
#include "talvos/Instruction.h"
#include "talvos/Invocation.h"
#include "talvos/Memory.h"
#include "talvos/Module.h"
#include "talvos/PipelineStage.h"
#include "talvos/RenderPass.h"
#include "talvos/Type.h"
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

  /// Location variables (key is {Location, Component}).
  std::map<std::pair<uint32_t, uint32_t>, Object> Locations;
};

/// Point primitive data, used for rasterization.
struct PipelineExecutor::PointPrimitive
{
  float X;         ///< The framebuffer x-coordinate.
  float Y;         ///< The framebuffer y-coordinate.
  float PointSize; ///< The point size.

  const VertexOutput &Out; ///< The vertex shader output.
};

/// Triangle primitive data, used for rasterization.
struct PipelineExecutor::TrianglePrimitive
{
  Vec4 PosA; ///< The position of vertex A.
  Vec4 PosB; ///< The position of vertex B.
  Vec4 PosC; ///< The position of vertex C.

  const VertexOutput &OutA; ///< The vertex shader outputs for vertex A.
  const VertexOutput &OutB; ///< The vertex shader outputs for vertex B.
  const VertexOutput &OutC; ///< The vertex shader outputs for vertex C.
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
        uint32_t LocalIndex = LX + (LY + (LZ * GroupSize.Y)) * GroupSize.X;
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
          case SpvBuiltInLocalInvocationIndex:
            PipelineMemory->store(Address, Sz, (uint8_t *)&LocalIndex);
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

  const PipelineContext &PC = Cmd.getPipelineContext();
  const ComputePipeline *PL = PC.getComputePipeline();
  assert(PL != nullptr);
  CurrentStage = PL->getStage();

  // Allocate and initialize push constant data.
  Memory &GlobalMem = Dev.getGlobalMemory();
  uint64_t PushConstantAddress =
      GlobalMem.allocate(PipelineContext::PUSH_CONSTANT_MEM_SIZE);
  GlobalMem.store(PushConstantAddress, PipelineContext::PUSH_CONSTANT_MEM_SIZE,
                  PC.getPushConstantData());

  Objects = CurrentStage->getObjects();
  initializeVariables(PC.getComputeDescriptors(), PushConstantAddress);

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

  // Run worker threads to process groups.
  NextWorkIndex = 0;
  runWorkers(
      [&]() { return std::thread(&PipelineExecutor::runComputeWorker, this); });

  finalizeVariables(PC.getComputeDescriptors());
  GlobalMem.release(PushConstantAddress);

  PendingGroups.clear();
  CurrentCommand = nullptr;
}

void PipelineExecutor::run(const talvos::DrawCommandBase &Cmd)
{
  assert(CurrentCommand == nullptr);
  CurrentCommand = &Cmd;

  Continue = false;
  Interactive = checkEnv("TALVOS_INTERACTIVE", false);

  const PipelineContext &PC = Cmd.getPipelineContext();
  const GraphicsPipeline *PL = PC.getGraphicsPipeline();
  assert(PL != nullptr);

  // Get selected viewport.
  // TODO: Handle multiple viewports (and ViewportIndex)
  assert(Cmd.getPipelineContext().getViewports().size() == 1);
  VkViewport Viewport = Cmd.getPipelineContext().getViewports()[0];

  // Allocate and initialize push constant data.
  Memory &GlobalMem = Dev.getGlobalMemory();
  uint64_t PushConstantAddress =
      GlobalMem.allocate(PipelineContext::PUSH_CONSTANT_MEM_SIZE);
  GlobalMem.store(PushConstantAddress, PipelineContext::PUSH_CONSTANT_MEM_SIZE,
                  PC.getPushConstantData());

  // Set up vertex shader stage pipeline memories.
  RenderPipelineState State;
  State.VertexOutputs.resize(Cmd.getNumVertices());

  // Loop over instances.
  for (uint32_t Instance = 0; Instance < Cmd.getNumInstances(); Instance++)
  {
    uint32_t InstanceIndex = Instance + Cmd.getInstanceOffset();

    // Prepare vertex stage objects.
    CurrentStage = PL->getVertexStage();
    Objects = CurrentStage->getObjects();
    initializeVariables(PC.getGraphicsDescriptors(), PushConstantAddress);

    // Run worker threads to process vertices.
    NextWorkIndex = 0;
    runWorkers([&]() {
      return std::thread(&PipelineExecutor::runVertexWorker, this, &State,
                         InstanceIndex);
    });

    finalizeVariables(PC.getGraphicsDescriptors());

    // Switch to fragment shader for rasterization.
    CurrentStage = PL->getFragmentStage();
    assert(CurrentStage && "rendering without fragment shader not implemented");
    Objects = CurrentStage->getObjects();
    initializeVariables(PC.getGraphicsDescriptors(), PushConstantAddress);

    // TODO: Handle other topologies
    VkPrimitiveTopology Topology = PL->getTopology();
    switch (Topology)
    {
    case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
    {
      for (uint32_t v = 0; v < Cmd.getNumVertices(); v++)
        rasterizePoint(Cmd, Viewport, State.VertexOutputs[v]);
      break;
    }
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
    {
      for (uint32_t v = 0; v < Cmd.getNumVertices(); v += 3)
        rasterizeTriangle(Cmd, Viewport, State.VertexOutputs[v],
                          State.VertexOutputs[v + 1],
                          State.VertexOutputs[v + 2]);
      break;
    }
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
    {
      for (uint32_t v = 2; v < Cmd.getNumVertices(); v++)
      {
        const VertexOutput &A = State.VertexOutputs[v - 2];
        const VertexOutput &B = State.VertexOutputs[v - 1];

        const VertexOutput &C = State.VertexOutputs[v];
        rasterizeTriangle(Cmd, Viewport, A, B, C);

        if (++v >= Cmd.getNumVertices())
          break;

        const VertexOutput &D = State.VertexOutputs[v];
        rasterizeTriangle(Cmd, Viewport, B, D, C);
      }
      break;
    }
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
    {
      const VertexOutput &Center = State.VertexOutputs[0];
      for (uint32_t v = 2; v < Cmd.getNumVertices(); v++)
      {
        const VertexOutput &A = State.VertexOutputs[v - 1];
        const VertexOutput &B = State.VertexOutputs[v];
        rasterizeTriangle(Cmd, Viewport, Center, A, B);
      }
      break;
    }
    default:
      std::cerr << "Unimplemented primitive topology: " << Topology
                << std::endl;
      abort();
    }

    finalizeVariables(PC.getGraphicsDescriptors());
  }

  GlobalMem.release(PushConstantAddress);

  CurrentStage = nullptr;
  CurrentCommand = nullptr;
}

void PipelineExecutor::runWorkers(std::function<std::thread()> ThreadCreator)
{
  // Get number of worker threads to launch.
  NumThreads = 1;
  if (!Interactive && Dev.isThreadSafe())
    NumThreads = (uint32_t)getEnvUInt("TALVOS_NUM_WORKERS",
                                      std::thread::hardware_concurrency());

  // Create worker threads.
  std::vector<std::thread> Threads;
  for (unsigned i = 0; i < NumThreads; i++)
    Threads.push_back(ThreadCreator());

  // Wait for workers to complete.
  for (unsigned i = 0; i < NumThreads; i++)
    Threads[i].join();
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

void PipelineExecutor::buildPendingFragments(const DrawCommandBase &Cmd,
                                             int XMinFB, int XMaxFB, int YMinFB,
                                             int YMaxFB)
{
  const RenderPassInstance &RPI = Cmd.getRenderPassInstance();
  const Framebuffer &FB = RPI.getFramebuffer();
  const PipelineContext &PC = Cmd.getPipelineContext();

  // Clamp the bounding box to be within the framebuffer.
  XMinFB = std::clamp(XMinFB, 0, (int)(FB.getWidth() - 1));
  XMaxFB = std::clamp(XMaxFB, 0, (int)(FB.getWidth() - 1));
  YMinFB = std::clamp(YMinFB, 0, (int)(FB.getHeight() - 1));
  YMaxFB = std::clamp(YMaxFB, 0, (int)(FB.getHeight() - 1));

  // Clamp the bounding box to be within the scissor rectangle.
  // TODO: Select correct scissor for current viewport
  assert(PC.getScissors().size() == 1);
  VkRect2D Scissor = PC.getScissors()[0];
  XMinFB = std::max<int>(XMinFB, Scissor.offset.x);
  XMaxFB = std::min<int>(XMaxFB, Scissor.offset.x + Scissor.extent.width - 1);
  YMinFB = std::max<int>(YMinFB, Scissor.offset.y);
  YMaxFB = std::min<int>(YMaxFB, Scissor.offset.y + Scissor.extent.height - 1);

  // Build list of framebuffer coordinates in the axis-aligned bounding box.
  assert(PendingFragments.empty());
  for (int YFB = YMinFB; YFB <= YMaxFB; YFB++)
    for (int XFB = XMinFB; XFB <= XMaxFB; XFB++)
      PendingFragments.push_back({(uint32_t)XFB, (uint32_t)YFB, 0});
}

/// Recursively populate a fragment shader input variable by interpolating
/// between the vertex shader output variables in a triangle.
///
/// \param Output       The object being populated.
/// \param Ty           The current type.
/// \param Offset       The current byte offset within the object.
/// \param FA,FB,FC     The vertex shader output variables.
/// \param AW,BW,CW     The clip w coordinates of the vertices.
/// \param InvW         The inverse of the interpolated clip w coordinate.
/// \param a,b,c        The barycentric coordinates of the fragment.
/// \param Flat         True to signal flat shading.
/// \param Perspective  True to signal perspective-correct interpolation.
void interpolate(Object &Output, const Type *Ty, size_t Offset,
                 const Object &FA, const Object &FB, const Object &FC, float AW,
                 float BW, float CW, float InvW, float a, float b, float c,
                 bool Flat, bool Perspective)
{
  if (Ty->isScalar())
  {
    if (Flat)
    {
      // Copy data from provoking vertex.
      memcpy(Output.getData() + Offset, FA.getData() + Offset, Ty->getSize());
      return;
    }

    // Interpolation requires 32-bit floating point values.
    assert(Ty->isFloat() && Ty->getBitWidth() == 32);

    // Interpolate scalar values between vertices.
    float A = *(float *)(FA.getData() + Offset);
    float B = *(float *)(FB.getData() + Offset);
    float C = *(float *)(FC.getData() + Offset);
    float F;
    if (Perspective)
      F = ((a * A / AW) + (b * B / BW) + (c * C / CW)) / InvW;
    else
      F = (a * A) + (b * B) + (c * C);

    *(float *)(Output.getData() + Offset) = F;
    return;
  }

  // Recurse through aggregate members.
  for (uint32_t i = 0; i < Ty->getElementCount(); i++)
  {
    // Check for Flat and NoPerspective member decorations.
    bool FlatElement = Flat;
    bool PerspectiveElement = Perspective;
    if (Ty->getTypeId() == Type::STRUCT)
    {
      if (Ty->getStructMemberDecorations(i).count(SpvDecorationFlat))
        FlatElement = true;
      if (Ty->getStructMemberDecorations(i).count(SpvDecorationNoPerspective))
        PerspectiveElement = false;
    }

    interpolate(Output, Ty->getElementType(i), Offset + Ty->getElementOffset(i),
                FA, FB, FC, AW, BW, CW, InvW, a, b, c, FlatElement,
                PerspectiveElement);
  }
}

float XDevToFB(float Xd, VkViewport Viewport)
{
  return ((Viewport.width / 2.f) * Xd) + (Viewport.x + Viewport.width / 2.f);
}

float XFBToDev(float Xfb, VkViewport Viewport)
{
  return ((Xfb + 0.5f) - (Viewport.x + Viewport.width / 2.f)) /
         (Viewport.width / 2.f);
}

float YDevToFB(float Yd, VkViewport Viewport)
{
  return ((Viewport.height / 2.f) * Yd) + (Viewport.y + Viewport.height / 2.f);
}

float YFBToDev(float Yfb, VkViewport Viewport)
{
  return ((Yfb + 0.5f) - (Viewport.y + Viewport.height / 2.f)) /
         (Viewport.height / 2.f);
}

void PipelineExecutor::processFragment(
    const Fragment &Frag, const RenderPassInstance &RPI,
    std::function<void(uint32_t, uint32_t, const Variable *, const Type *,
                       Memory *, uint64_t)>
        GenLocData)
{
  const Framebuffer &FB = RPI.getFramebuffer();
  const RenderPass &RP = RPI.getRenderPass();

  // Data about a fragment shader output variable.
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
    const Type *PtrTy = Var->getType();
    const Type *VarTy = PtrTy->getElementType();
    if (PtrTy->getStorageClass() == SpvStorageClassInput)
    {
      // Allocate storage for input variable.
      uint64_t Address = PipelineMemory->allocate(VarTy->getSize());
      InitialObjects[Var->getId()] = Object(PtrTy, Address);

      // Initialize input variable data.
      if (Var->hasDecoration(SpvDecorationLocation))
      {
        uint32_t Location = Var->getDecoration(SpvDecorationLocation);
        uint32_t Component = 0;
        if (Var->hasDecoration(SpvDecorationComponent))
          Component = Var->getDecoration(SpvDecorationComponent);
        GenLocData(Location, Component, Var, VarTy, &*PipelineMemory, Address);
      }
      else if (Var->hasDecoration(SpvDecorationBuiltIn))
      {
        switch (Var->getDecoration(SpvDecorationBuiltIn))
        {
        case SpvBuiltInFragCoord:
        {
          // TODO: Sample shading affects x/y components
          assert(VarTy->isVector() && VarTy->getSize() == 16);
          float FragCoord[4] = {Frag.X + 0.5f, Frag.Y + 0.5f, Frag.Depth,
                                Frag.InvW};
          PipelineMemory->store(Address, 16, (const uint8_t *)FragCoord);
          break;
        }
        default:
          assert(false && "Unhandled fragment input builtin");
        }
      }
      else
      {
        assert(false && "Unhandled input variable type");
      }
    }
    else if (PtrTy->getStorageClass() == SpvStorageClassOutput)
    {
      // Allocate storage for output variable.
      uint64_t Address = PipelineMemory->allocate(VarTy->getSize());
      InitialObjects[Var->getId()] = Object(PtrTy, Address);

      // Store output variable information.
      assert(Var->hasDecoration(SpvDecorationLocation));
      uint32_t Location = Var->getDecoration(SpvDecorationLocation);
      uint32_t Component = 0;
      if (Var->hasDecoration(SpvDecorationComponent))
        Component = Var->getDecoration(SpvDecorationComponent);
      Outputs[Var] = {Address, Location, Component};
    }
  }

  // Create fragment shader invocation.
  CurrentInvocation = new Invocation(Dev, *CurrentStage, InitialObjects,
                                     PipelineMemory, nullptr, Dim3(0, 0, 0));

  // Run shader invocation to completion.
  interact();
  while (CurrentInvocation->getState() == Invocation::READY)
  {
    CurrentInvocation->step();
    interact();
  }

  bool Discarded = CurrentInvocation->wasDiscarded();

  delete CurrentInvocation;
  CurrentInvocation = nullptr;

  if (Discarded)
    return;

  // Gather fragment outputs for each location.
  std::vector<uint32_t> ColorAttachments =
      RP.getSubpass(RPI.getSubpassIndex()).ColorAttachments;
  std::map<uint32_t, Image::Texel> OutTexels;
  for (auto Output : Outputs)
  {
    uint32_t Location = Output.second.Location;
    assert(Location < ColorAttachments.size());

    // Get output variable data.
    const Object &OutputData =
        Object::load(Output.first->getType()->getElementType(), *PipelineMemory,
                     Output.second.Address);

    // Set texel component(s) for this variable.
    assert(OutputData.getType()->isScalar() ||
           OutputData.getType()->isVector());
    assert(OutputData.getType()->getScalarType()->getSize() == 4);
    Image::Texel T;
    if (OutTexels.count(Location))
      T = OutTexels.at(Location);
    for (uint32_t i = 0; i < OutputData.getType()->getElementCount(); i++)
      T.set(Output.second.Component + i, OutputData.get<uint32_t>(i));
    OutTexels[Location] = T;
  }

  // Write fragment outputs to color attachments.
  for (auto OT : OutTexels)
  {
    uint32_t Ref = ColorAttachments[OT.first];
    assert(Ref < RP.getNumAttachments());
    assert(Ref < FB.getAttachments().size());

    // Write pixel color to attachment.
    const ImageView *Attach = FB.getAttachments()[Ref];
    Attach->write(OT.second, Frag.X, Frag.Y);
  }
}

void PipelineExecutor::runPointFragmentWorker(PointPrimitive Primitive,
                                              const RenderPassInstance &RPI)
{
  IsWorkerThread = true;
  CurrentInvocation = nullptr;

  // Loop until all framebuffer coordinates have been processed.
  while (true)
  {
    // Get next framebuffer coordinate index.
    uint32_t WorkIndex = (uint32_t)NextWorkIndex++;
    if (WorkIndex >= PendingFragments.size())
      break;

    Fragment Frag;
    Frag.X = PendingFragments[WorkIndex].X;
    Frag.Y = PendingFragments[WorkIndex].Y;
    Frag.Depth = 0; // TODO
    Frag.InvW = 0;  // TODO

    // Compute point coordinate.
    float S = 0.5f + (Frag.X + 0.5f - Primitive.X) / Primitive.PointSize;
    float T = 0.5f + (Frag.Y + 0.5f - Primitive.Y) / Primitive.PointSize;

    // Check if pixel is inside point radius.
    if (S < 0 || T < 0 || S > 1 || T > 1)
      continue;

    // Lambda for generating data for location variables.
    auto GenLocData = [&](uint32_t Location, uint32_t Component,
                          const Variable *Var, const Type *VarTy, Memory *Mem,
                          uint64_t Address) {
      const Object &Out = Primitive.Out.Locations.at({Location, Component});
      Mem->store(Address, VarTy->getSize(), Out.getData());
    };

    processFragment(Frag, RPI, GenLocData);
  }
}

void PipelineExecutor::runTriangleFragmentWorker(TrianglePrimitive Primitive,
                                                 const PipelineContext &PC,
                                                 const RenderPassInstance &RPI,
                                                 const VkViewport &Viewport)
{
  IsWorkerThread = true;
  CurrentInvocation = nullptr;

  // Get rasterization state.
  const VkPipelineRasterizationStateCreateInfo &RasterizationState =
      PC.getGraphicsPipeline()->getRasterizationState();

  // Get vertex positions.
  Vec4 A = Primitive.PosA;
  Vec4 B = Primitive.PosB;
  Vec4 C = Primitive.PosC;

  // Loop until all framebuffer coordinates have been processed.
  while (true)
  {
    // Get next framebuffer coordinate index.
    uint32_t WorkIndex = (uint32_t)NextWorkIndex++;
    if (WorkIndex >= PendingFragments.size())
      break;

    Fragment Frag;
    Frag.X = PendingFragments[WorkIndex].X;
    Frag.Y = PendingFragments[WorkIndex].Y;

    // Compute the area of a triangle (doubled).
    auto TriArea2 = [](const Vec4 &A, const Vec4 &B, const Vec4 &C) {
      return (C.X - A.X) * (B.Y - A.Y) - (B.X - A.X) * (C.Y - A.Y);
    };

    // Compute barycentric coordinates using normalized device coordinates.
    Vec4 DevCoord = {XFBToDev(Frag.X, Viewport), YFBToDev(Frag.Y, Viewport)};
    float Area2 = TriArea2(A, B, C);
    float a = TriArea2(B, C, DevCoord) / Area2;
    float b = TriArea2(C, A, DevCoord) / Area2;
    float c = TriArea2(A, B, DevCoord) / Area2;

    // Check if pixel is inside triangle.
    if (!(a >= 0 && b >= 0 && c >= 0))
      continue;

    // Determine whether triangle is front-facing.
    bool FrontFacing;
    switch (RasterizationState.frontFace)
    {
    case VK_FRONT_FACE_COUNTER_CLOCKWISE:
      FrontFacing = Area2 > 0;
      break;
    case VK_FRONT_FACE_CLOCKWISE:
      FrontFacing = Area2 < 0;
      break;
    default:
      std::cerr << "Invalid front-facing sign value" << std::endl;
      abort();
    }

    // Cull triangle if necessary.
    if (FrontFacing && RasterizationState.cullMode & VK_CULL_MODE_FRONT_BIT)
      continue;
    if (!FrontFacing && RasterizationState.cullMode & VK_CULL_MODE_BACK_BIT)
      continue;

    // Calculate edge vectors.
    float BCX = C.X - B.X;
    float BCY = C.Y - B.Y;
    float CAX = A.X - C.X;
    float CAY = A.Y - C.Y;
    float ABX = B.X - A.X;
    float ABY = B.Y - A.Y;
    if (!FrontFacing)
    {
      BCX = -BCX;
      BCY = -BCY;
      CAX = -CAX;
      CAY = -CAY;
      ABX = -ABX;
      ABY = -ABY;
    }

    // Only fill top-left edges to avoid double-sampling on shared edges.
    if (a == 0)
    {
      if (!((BCY == 0 && BCX < 0) || BCY > 0))
        continue;
    }
    if (b == 0)
    {
      if (!((CAY == 0 && CAX < 0) || CAY > 0))
        continue;
    }
    if (c == 0)
    {
      if (!((ABY == 0 && ABX < 0) || ABY > 0))
        continue;
    }

    // Compute fragment depth and 1/w using linear interpolation.
    Frag.Depth = (a * A.Z) + (b * B.Z) + (c * C.Z);
    Frag.InvW = (a / A.W) + (b / B.W) + (c / C.W);

    // Lambda for generating data for location variables.
    auto GenLocData = [&](uint32_t Location, uint32_t Component,
                          const Variable *Var, const Type *VarTy, Memory *Mem,
                          uint64_t Address) {
      // Gather output data from each vertex.
      const Object &FA = Primitive.OutA.Locations.at({Location, Component});
      const Object &FB = Primitive.OutB.Locations.at({Location, Component});
      const Object &FC = Primitive.OutC.Locations.at({Location, Component});

      // Interpolate vertex outputs to produce fragment input.
      Object VarObj(VarTy);
      interpolate(VarObj, VarTy, 0, FA, FB, FC, A.W, B.W, C.W, Frag.InvW, a, b,
                  c, Var->hasDecoration(SpvDecorationFlat),
                  !Var->hasDecoration(SpvDecorationNoPerspective));
      VarObj.store(*Mem, Address);
    };

    processFragment(Frag, RPI, GenLocData);
  }
}

void PipelineExecutor::runVertexWorker(struct RenderPipelineState *State,
                                       uint32_t InstanceIndex)
{
  IsWorkerThread = true;
  CurrentInvocation = nullptr;

  const DrawCommandBase *DC = (const DrawCommandBase *)CurrentCommand;

  // Loop until all vertices are finished.
  while (true)
  {
    // Get next vertex index.
    uint32_t WorkIndex = (uint32_t)NextWorkIndex++;
    if (WorkIndex >= DC->getNumVertices())
      break;

    // Generate vertex index from work index.
    uint32_t VertexIndex;
    switch (DC->getType())
    {
    case Command::DRAW:
      VertexIndex = WorkIndex + DC->getVertexOffset();
      break;
    case Command::DRAW_INDEXED:
    {
      // Load index from memory.
      const DrawIndexedCommand *DIC = (const DrawIndexedCommand *)DC;
      uint64_t BaseAddress = DIC->getIndexBaseAddress();
      switch (DIC->getIndexType())
      {
      case VK_INDEX_TYPE_UINT16:
      {
        uint16_t VertexIndex16;
        Dev.getGlobalMemory().load(
            (uint8_t *)&VertexIndex16,
            BaseAddress + (WorkIndex + DIC->getIndexOffset()) * 2, 2);
        VertexIndex = VertexIndex16;
        break;
      }
      case VK_INDEX_TYPE_UINT32:
        Dev.getGlobalMemory().load(
            (uint8_t *)&VertexIndex,
            BaseAddress + (WorkIndex + DIC->getIndexOffset()) * 4, 4);
        break;
      default:
        assert(false && "Unhandled vertex index type");
        VertexIndex = UINT32_MAX;
        break;
      }
      VertexIndex += DC->getVertexOffset();
      break;
    }
    default:
      assert(false && "Unhandled draw type");
    }

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
        const Type *ElemTy = Ty->getElementType();
        size_t ElemSize = ElemTy->getSize();
        uint64_t Address = PipelineMemory->allocate(ElemSize);
        InitialObjects[Var->getId()] = Object(Ty, Address);

        // Initialize input variable data.
        if (Var->hasDecoration(SpvDecorationLocation))
        {
          uint32_t Location = Var->getDecoration(SpvDecorationLocation);
          uint32_t Component = 0;
          if (Var->hasDecoration(SpvDecorationComponent))
            Component = Var->getDecoration(SpvDecorationComponent);

          if (ElemTy->isMatrix())
          {
            assert(Component == 0);

            const Type *ColTy = ElemTy->getElementType();
            size_t ColSize = ColTy->getSize();

            // Each matrix column occupies a distinct location.
            for (uint32_t Col = 0; Col < ElemTy->getElementCount(); Col++)
            {
              loadVertexInput(DC->getPipelineContext(), &*PipelineMemory,
                              Address + Col * ColSize, VertexIndex,
                              InstanceIndex, Location + Col, 0, ColTy);
            }
          }
          else
          {
            loadVertexInput(DC->getPipelineContext(), &*PipelineMemory, Address,
                            VertexIndex, InstanceIndex, Location, Component,
                            ElemTy);
          }
        }
        else if (Var->hasDecoration(SpvDecorationBuiltIn))
        {
          switch (Var->getDecoration(SpvDecorationBuiltIn))
          {
          case SpvBuiltInInstanceIndex:
            PipelineMemory->store(Address, 4, (const uint8_t *)&InstanceIndex);
            break;
          case SpvBuiltInVertexIndex:
            assert(ElemSize == 4);
            PipelineMemory->store(Address, 4, (const uint8_t *)&VertexIndex);
            break;
          default:
            assert(false && "Unhandled vertex input builtin");
          }
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
    for (auto Var : CurrentStage->getEntryPoint()->getVariables())
    {
      if (Var->getType()->getStorageClass() != SpvStorageClassOutput)
        continue;

      uint64_t BaseAddress = OutputAddresses[Var];
      const Type *Ty = Var->getType()->getElementType();

      if (Var->hasDecoration(SpvDecorationBuiltIn))
      {
        SpvBuiltIn BuiltIn =
            (SpvBuiltIn)Var->getDecoration(SpvDecorationBuiltIn);
        State->VertexOutputs[WorkIndex].BuiltIns[BuiltIn] =
            Object::load(Ty, *PipelineMemory, BaseAddress);
      }
      else if (Var->hasDecoration(SpvDecorationLocation))
      {
        uint32_t Location = Var->getDecoration(SpvDecorationLocation);
        uint32_t Component = 0;
        if (Var->hasDecoration(SpvDecorationComponent))
          Component = Var->getDecoration(SpvDecorationComponent);
        State->VertexOutputs[WorkIndex].Locations[{Location, Component}] =
            Object::load(Ty, *PipelineMemory, BaseAddress);
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
          State->VertexOutputs[WorkIndex].BuiltIns[BuiltIn] =
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

void PipelineExecutor::finalizeVariables(const DescriptorSetMap &DSM)
{
  // Copy array variable data back to original buffers.
  for (auto V : CurrentStage->getModule()->getVariables())
  {
    // TODO: Skip constant variables too
    if (!V->isBufferVariable())
      continue;
    if (V->getType()->getElementType()->getTypeId() != Type::ARRAY)
      continue;

    assert(V->getType()->getElementType()->getTypeId() == Type::ARRAY);
    const Type *ArrayTy = V->getType()->getElementType();

    // Get descriptor set and binding.
    uint32_t Set = V->getDecoration(SpvDecorationDescriptorSet);
    uint32_t Binding = V->getDecoration(SpvDecorationBinding);
    assert(DSM.count(Set));

    uint64_t Address = Objects[V->getId()].get<uint64_t>();
    const DescriptorElement *DescriptorElements =
        Objects[V->getId()].getDescriptorElements();
    assert(DescriptorElements);

    // Copy array element values to original buffers.
    for (uint32_t i = 0; i < ArrayTy->getElementCount(); i++)
    {
      if (!DSM.at(Set).count({Binding, i}))
        continue;

      Memory::copy(DSM.at(Set).at({Binding, i}).Address, Dev.getGlobalMemory(),
                   DescriptorElements[i].Address, Dev.getGlobalMemory(),
                   DescriptorElements[i].NumBytes);
    }

    // Release allocation.
    Dev.getGlobalMemory().release(Address);
    delete[] DescriptorElements;
  }
}

void PipelineExecutor::initializeVariables(const talvos::DescriptorSetMap &DSM,
                                           uint64_t PushConstantAddress)
{
  for (auto V : CurrentStage->getModule()->getVariables())
  {
    // Set push constant data address.
    if (V->getType()->getStorageClass() == SpvStorageClassPushConstant)
    {
      Objects[V->getId()] = Object(V->getType(), PushConstantAddress);
      continue;
    }

    if (!V->isBufferVariable())
      continue;

    // Look up variable in descriptor set and set pointer value if present.
    uint32_t Set = V->getDecoration(SpvDecorationDescriptorSet);
    uint32_t Binding = V->getDecoration(SpvDecorationBinding);
    if (!DSM.count(Set))
      continue;

    if (V->getType()->getElementType()->getTypeId() == Type::ARRAY)
    {
      const Type *ArrayType = V->getType()->getElementType();

      // Allocate array for descriptor element information.
      DescriptorElement *DescriptorElements =
          new DescriptorElement[ArrayType->getElementCount()];

      // Determine offset for each array element and total size.
      uint64_t NumBytes = 0;
      for (uint32_t i = 0; i < ArrayType->getElementCount(); i++)
      {
        if (!DSM.at(Set).count({Binding, i}))
        {
          DescriptorElements[i] = {0, 0};
          continue;
        }

        size_t ElemSize = DSM.at(Set).at({Binding, i}).NumBytes;
        DescriptorElements[i] = {NumBytes, ElemSize};
        NumBytes += (uint64_t)ElemSize;
      }

      // Create new allocation to store whole array.
      uint64_t Address = Dev.getGlobalMemory().allocate(NumBytes);
      Objects[V->getId()] = Object(V->getType(), Address);
      Objects[V->getId()].setDescriptorElements(DescriptorElements);

      // Copy array element values into new allocation.
      for (uint32_t i = 0; i < ArrayType->getElementCount(); i++)
      {
        if (!DSM.at(Set).count({Binding, i}))
          continue;

        // Set final address for start of array element.
        DescriptorElements[i].Address = Address + DescriptorElements[i].Address;

        // Perform copy.
        Memory::copy(DescriptorElements[i].Address, Dev.getGlobalMemory(),
                     DSM.at(Set).at({Binding, i}).Address,
                     Dev.getGlobalMemory(), DescriptorElements[i].NumBytes);
      }
    }
    else
    {
      if (!DSM.at(Set).count({Binding, 0}))
        continue;

      Objects[V->getId()] =
          Object(V->getType(), DSM.at(Set).at({Binding, 0}).Address);
    }
  }
}

void PipelineExecutor::rasterizePoint(const DrawCommandBase &Cmd,
                                      const VkViewport &Viewport,
                                      const VertexOutput &Vertex)
{
  const RenderPassInstance &RPI = Cmd.getRenderPassInstance();

  // Get the point position.
  Vec4 Position = getPosition(Vertex);

  // Get the point size.
  float PointSize = 0.1f;
  if (Vertex.BuiltIns.count(SpvBuiltInPointSize))
    PointSize = Vertex.BuiltIns.at(SpvBuiltInPointSize).get<float>();

  // Get framebuffer coordinate of primitive.
  float X = XDevToFB(Position.X, Viewport);
  float Y = YDevToFB(Position.Y, Viewport);

  // Compute a bounding box for the point primitive.
  int XMinFB = (int)std::floor(X - (PointSize / 2));
  int XMaxFB = (int)std::ceil(X + (PointSize / 2));
  int YMinFB = (int)std::floor(Y - (PointSize / 2));
  int YMaxFB = (int)std::ceil(Y + (PointSize / 2));

  buildPendingFragments(Cmd, XMinFB, XMaxFB, YMinFB, YMaxFB);

  // Run worker threads to process fragments.
  NextWorkIndex = 0;
  PointPrimitive Primitive = {X, Y, PointSize, Vertex};
  runWorkers([&]() {
    return std::thread(&PipelineExecutor::runPointFragmentWorker, this,
                       Primitive, RPI);
  });

  PendingFragments.clear();
}

void PipelineExecutor::rasterizeTriangle(const DrawCommandBase &Cmd,
                                         const VkViewport &Viewport,
                                         const VertexOutput &VA,
                                         const VertexOutput &VB,
                                         const VertexOutput &VC)
{
  const RenderPassInstance &RPI = Cmd.getRenderPassInstance();

  // Gather vertex positions for the primitive.
  Vec4 A = getPosition(VA);
  Vec4 B = getPosition(VB);
  Vec4 C = getPosition(VC);

  // Convert clip coordinates to normalized device coordinates.
  A.X /= A.W;
  A.Y /= A.W;
  A.Z /= A.W;
  B.X /= B.W;
  B.Y /= B.W;
  B.Z /= B.W;
  C.X /= C.W;
  C.Y /= C.W;
  C.Z /= C.W;

  // Compute an axis-aligned bounding box for the primitive.
  float XMinDev = std::fmin(A.X, std::fmin(B.X, C.X));
  float YMinDev = std::fmin(A.Y, std::fmin(B.Y, C.Y));
  float XMaxDev = std::fmax(A.X, std::fmax(B.X, C.X));
  float YMaxDev = std::fmax(A.Y, std::fmax(B.Y, C.Y));
  int XMinFB = (int)std::floor(XDevToFB(XMinDev, Viewport));
  int XMaxFB = (int)std::ceil(XDevToFB(XMaxDev, Viewport));
  int YMinFB = (int)std::floor(YDevToFB(YMinDev, Viewport));
  int YMaxFB = (int)std::ceil(YDevToFB(YMaxDev, Viewport));

  buildPendingFragments(Cmd, XMinFB, XMaxFB, YMinFB, YMaxFB);

  // Run worker threads to process fragments.
  NextWorkIndex = 0;
  TrianglePrimitive Primitive = {A, B, C, VA, VB, VC};
  runWorkers([&]() {
    return std::thread(&PipelineExecutor::runTriangleFragmentWorker, this,
                       Primitive, Cmd.getPipelineContext(), RPI, Viewport);
  });

  PendingFragments.clear();
}

void PipelineExecutor::signalError()
{
  if (!IsWorkerThread)
    return;

  // Drop to interactive prompt.
  Continue = false;
  interact();
}

Vec4 PipelineExecutor::getPosition(const VertexOutput &Out)
{
  Vec4 Pos;
  assert(Out.BuiltIns.count(SpvBuiltInPosition));
  const Object &PosObj = Out.BuiltIns.at(SpvBuiltInPosition);
  assert(PosObj.getType()->isVector() &&
         PosObj.getType()->getElementType()->isFloat() &&
         PosObj.getType()->getElementType()->getBitWidth() == 32 &&
         "Position built-in type must be float4");
  memcpy(&Pos, PosObj.getData(), sizeof(Vec4));
  return Pos;
}

void PipelineExecutor::loadVertexInput(const PipelineContext &PC,
                                       Memory *PipelineMemory, uint64_t Address,
                                       uint32_t VertexIndex,
                                       uint32_t InstanceIndex,
                                       uint32_t Location, uint32_t Component,
                                       const Type *ElemTy) const
{
  const GraphicsPipeline *Pipeline = PC.getGraphicsPipeline();
  assert(Pipeline != nullptr);

  // Get vertex attribute description.
  auto &Attributes = Pipeline->getVertexAttributeDescriptions();
  auto Attr =
      std::find_if(Attributes.begin(), Attributes.end(),
                   [Location](auto Elem) { return Elem.location == Location; });
  assert(Attr != Attributes.end() && "invalid attribute location");

  // Get vertex binding description.
  auto &Bindings = Pipeline->getVertexBindingDescriptions();
  auto Binding =
      std::find_if(Bindings.begin(), Bindings.end(),
                   [Attr](auto Elem) { return Elem.binding == Attr->binding; });
  assert(Binding != Bindings.end() && "invalid binding number");

  // TODO: Handle other formats
  assert(Attr->format == VK_FORMAT_R32G32B32A32_SFLOAT ||
         Attr->format == VK_FORMAT_R32G32B32_SFLOAT ||
         Attr->format == VK_FORMAT_R32G32_SFLOAT ||
         Attr->format == VK_FORMAT_R32_SFLOAT ||
         Attr->format == VK_FORMAT_R32G32B32A32_SINT ||
         Attr->format == VK_FORMAT_R32G32B32_SINT ||
         Attr->format == VK_FORMAT_R32G32_SINT ||
         Attr->format == VK_FORMAT_R32_SINT ||
         Attr->format == VK_FORMAT_R32G32B32A32_UINT ||
         Attr->format == VK_FORMAT_R32G32B32_UINT ||
         Attr->format == VK_FORMAT_R32G32_UINT ||
         Attr->format == VK_FORMAT_R32_UINT);

  // Calculate variable address in vertex buffer memory.
  uint64_t ElemAddr = PC.getVertexBindings().at(Attr->binding);
  switch (Binding->inputRate)
  {
  case VK_VERTEX_INPUT_RATE_VERTEX:
    ElemAddr += VertexIndex * Binding->stride;
    break;
  case VK_VERTEX_INPUT_RATE_INSTANCE:
    ElemAddr += InstanceIndex * Binding->stride;
    break;
  default:
    assert(false && "Unhandled vertex input rate");
  }
  ElemAddr += Attr->offset;

  // Add offset for requested component.
  if (Component)
  {
    assert(ElemTy->isScalar() || ElemTy->isVector());
    ElemAddr += Component * ElemTy->getScalarType()->getSize();
  }

  // Set default values for the variable.
  // As per the Vulkan specification, if the G, B, or A components are
  // missing, they should be filled with (0,0,1) as needed,
  Object Default(ElemTy);
  Default.zero();
  if (ElemTy->isVector() && ElemTy->getElementCount() == 4)
  {
    const Type *ScalarTy = ElemTy->getElementType();
    if (ScalarTy->isFloat() && ScalarTy->getBitWidth() == 32)
      Default.set<float>(1.f, 3);
    else if (ScalarTy->isFloat() && ScalarTy->getBitWidth() == 64)
      Default.set<double>(1.0, 3);
    else if (ScalarTy->isInt() && ScalarTy->getBitWidth() == 16)
      Default.set<uint16_t>(1, 3);
    else if (ScalarTy->isInt() && ScalarTy->getBitWidth() == 32)
      Default.set<uint32_t>(1, 3);
    else if (ScalarTy->isInt() && ScalarTy->getBitWidth() == 64)
      Default.set<uint64_t>(1, 3);
    else
      assert(false && "Unhandled vertex input variable type");
  }
  Default.store(*PipelineMemory, Address);

  // Copy vertex input data to pipeline memory.
  Memory::copy(
      Address, *PipelineMemory, ElemAddr, Dev.getGlobalMemory(),
      std::min(ElemTy->getSize(), (size_t)getElementSize(Attr->format)));
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
