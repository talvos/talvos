// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <iostream>
#include <spirv/unified1/spirv.h>

#include "talvos/DispatchCommand.h"
#include "talvos/Function.h"
#include "talvos/Invocation.h"
#include "talvos/Memory.h"
#include "talvos/Module.h"
#include "talvos/Workgroup.h"

namespace talvos
{

DispatchCommand::DispatchCommand(Device *D, const Module *M, const Function *F,
                                 Dim3 NumGroups, const DescriptorSet &DS)
{
  Dev = D;
  Mod = M;
  Func = F;

  this->NumGroups = NumGroups;
  this->GroupSize = M->getLocalSize(F->getId());
  // TODO: Handle WorkgroupSize decoration

  // Resolve buffer variables.
  for (BufferVariableMap::value_type V : M->getBufferVariables())
  {
    // Look up variable in descriptor set and set pointer value.
    std::pair<uint32_t, uint32_t> Binding = {V.second.DescriptorSet,
                                             V.second.Binding};
    if (DS.count(Binding))
    {
      Object Pointer(V.second.Ty, DS.at(Binding));
      Variables.push_back({V.first, Pointer});
    }
  }
}

void DispatchCommand::run()
{
  for (uint32_t GZ = 0; GZ < NumGroups.Z; GZ++)
  {
    for (uint32_t GY = 0; GY < NumGroups.Y; GY++)
    {
      for (uint32_t GX = 0; GX < NumGroups.X; GX++)
      {
        Workgroup Group(this, {GX, GY, GZ});

        // Loop until all work items have completed.
        const Workgroup::WorkItemList &WorkItems = Group.getWorkItems();
        size_t NumWorkItems = WorkItems.size();
        while (true)
        {
          uint32_t BarrierCount = 0;

          // Step each invocation until it hits a barrier or completes.
          for (uint32_t I = 0; I < NumWorkItems; I++)
          {
            auto &WI = WorkItems[I];
            while (WI->getState() == Invocation::READY)
              WI->step();

            // Increment barrier count if necessary.
            if (WI->getState() == Invocation::BARRIER)
              BarrierCount++;
          }

          // Check for barriers.
          if (BarrierCount > 0)
          {
            // All invocations in the group must hit the barrier.
            // TODO: Ensure they hit the *same* barrier?
            // TODO: Allow for other execution scopes.
            if (BarrierCount != NumWorkItems)
            {
              // TODO: Better error message.
              std::cerr << "Barrier not reached by every invocation."
                        << std::endl;
              abort();
            }

            // Clear the barrier.
            for (auto &WI : WorkItems)
              WI->clearBarrier();
          }
          else
          {
            // All invocations must have completed - this group is done.
            break;
          }
        }
      }
    }
  }
}

} // namespace talvos
