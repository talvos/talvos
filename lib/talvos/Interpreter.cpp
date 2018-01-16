// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "talvos/Interpreter.h"
#include "talvos/Invocation.h"

namespace talvos
{

void interpret(const Module *M, const Function *F)
{
  // TODO: Launch more than one invocation
  Invocation I(M, F);

  // TODO: Handle barriers
  while (I.getState() == Invocation::READY)
  {
    I.step();
  }
}

} // namespace talvos
