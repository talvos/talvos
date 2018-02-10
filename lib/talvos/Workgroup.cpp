// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "talvos/Workgroup.h"
#include "talvos/Memory.h"

namespace talvos
{

Workgroup::Workgroup(Dim3 GroupId)
{
  this->GroupId = GroupId;
  LocalMemory = new Memory;
}

Workgroup::~Workgroup() { delete LocalMemory; }

} // namespace talvos
