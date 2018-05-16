// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Commands.cpp
/// This file defines the Command base class and its subclasses.

#include "talvos/Commands.h"
#include "PipelineExecutor.h"
#include "talvos/Device.h"

namespace talvos
{

void Command::run(Device &Dev) const
{
  Dev.reportCommandBegin(this);
  runImpl(Dev);
  Dev.reportCommandComplete(this);
}

void DispatchCommand::runImpl(Device &Dev) const
{
  Dev.getPipelineExecutor().run(*this);
}

} // namespace talvos
