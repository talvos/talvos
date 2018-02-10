// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <spirv/unified1/spirv.h>

#include "talvos/DispatchCommand.h"
#include "talvos/Function.h"
#include "talvos/Invocation.h"
#include "talvos/Module.h"

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
    // Look-up variable in descriptor set and set pointer value.
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
        for (uint32_t LZ = 0; LZ < GroupSize.Z; LZ++)
        {
          for (uint32_t LY = 0; LY < GroupSize.Y; LY++)
          {
            for (uint32_t LX = 0; LX < GroupSize.X; LX++)
            {
              Invocation I(this, {GX, GY, GZ}, {LX, LY, LZ}, Variables);

              // TODO: Handle barriers
              while (I.getState() == Invocation::READY)
              {
                I.step();
              }
            }
          }
        }
      }
    }
  }
}

} // namespace talvos
