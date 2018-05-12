// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Workgroup.cpp
/// This file defines the Workgroup class.

#include "talvos/Workgroup.h"
#include "PipelineExecutor.h"
#include "talvos/Invocation.h"
#include "talvos/Memory.h"
#include "talvos/Module.h"
#include "talvos/PipelineStage.h"

namespace talvos
{

Workgroup::Workgroup(Device &Dev, const PipelineExecutor &Executor,
                     Dim3 GroupId)
{
  this->GroupId = GroupId;
  LocalMemory = new Memory(Dev, MemoryScope::Workgroup);

  const PipelineStage &Stage = Executor.getCurrentStage();

  // Allocate workgroup variables.
  for (auto V : Stage.getModule()->getWorkgroupVariables())
  {
    size_t NumBytes = V.second->getElementType()->getSize();
    uint64_t Address = LocalMemory->allocate(NumBytes);
    Variables.push_back({V.first, Object(V.second, Address)});
  }
}

Workgroup::~Workgroup() { delete LocalMemory; }

void Workgroup::addWorkItem(std::unique_ptr<Invocation> WorkItem)
{
  WorkItems.push_back(std::move(WorkItem));
}

} // namespace talvos
