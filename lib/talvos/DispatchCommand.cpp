// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <iostream>

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

  // Resolve buffer variables.
  for (BufferVariableMap::value_type V : M->getBufferVariables())
  {
    // Look-up variable in descriptor set and set pointer value.
    std::pair<size_t, size_t> Binding = {V.second.DescriptorSet,
                                         V.second.Binding};
    if (DS.count(Binding))
    {
      Object Pointer = Object::create<size_t>(V.second.Ty, DS.at(Binding));
      Variables.push_back({V.first, Pointer});
    }
  }
}

DispatchCommand::~DispatchCommand()
{
  for (auto V : Variables)
    V.second.destroy();
}

void DispatchCommand::run()
{
  for (int Z = 0; Z < GroupCountZ; Z++)
  {
    for (int Y = 0; Y < GroupCountY; Y++)
    {
      for (int X = 0; X < GroupCountX; X++)
      {
        Invocation I(Dev, Mod, Func, X, Y, Z, Variables);

        // TODO: Handle barriers
        while (I.getState() == Invocation::READY)
        {
          I.step();
        }
      }
    }
  }
}

} // namespace talvos
