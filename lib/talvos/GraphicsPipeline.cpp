// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file GraphicsPipeline.cpp
/// This file defines the GraphicsPipeline class.

#include "talvos/GraphicsPipeline.h"
#include "talvos/PipelineStage.h"

namespace talvos
{

GraphicsPipeline::~GraphicsPipeline()
{
  delete VertexStage;
  delete FragmentStage;
}

} // namespace talvos
