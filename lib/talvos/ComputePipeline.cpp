// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file ComputePipeline.cpp
/// This file defines the ComputePipeline class.

#include "talvos/ComputePipeline.h"
#include "talvos/PipelineStage.h"

namespace talvos
{

ComputePipeline::~ComputePipeline() { delete Stage; }

} // namespace talvos
