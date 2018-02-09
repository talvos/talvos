// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <spirv/unified1/spirv.h>

#include "talvos/DispatchCommand.h"
#include "talvos/Invocation.h"
#include "talvos/Module.h"

namespace talvos
{

DispatchCommand::DispatchCommand(Device *D, const Module *M, const Function *F,
                                 uint32_t GroupCountX, uint32_t GroupCountY,
                                 uint32_t GroupCountZ, const DescriptorSet &DS)
{
  Dev = D;
  Mod = M;
  Func = F;
  this->GroupCountX = GroupCountX;
  this->GroupCountY = GroupCountY;
  this->GroupCountZ = GroupCountZ;

  // TODO: Use actual group size
  this->GroupSizeX = 1;
  this->GroupSizeY = 1;
  this->GroupSizeZ = 1;

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
  for (uint32_t GZ = 0; GZ < GroupCountZ; GZ++)
  {
    for (uint32_t GY = 0; GY < GroupCountY; GY++)
    {
      for (uint32_t GX = 0; GX < GroupCountX; GX++)
      {
        for (uint32_t LZ = 0; LZ < GroupSizeZ; LZ++)
        {
          for (uint32_t LY = 0; LY < GroupSizeY; LY++)
          {
            for (uint32_t LX = 0; LX < GroupSizeX; LX++)
            {
              Invocation I(this, GX, GY, GZ, LX, LY, LZ, Variables);

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
