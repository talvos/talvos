// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "vk_platform.h"
#include "vulkan_core.h"

// TODO: Remove this when all functions have implementations.
#include <cstdlib>
#include <iostream>
#define TALVOS_ABORT_UNIMPLEMENTED                                             \
  std::cerr << "Talvos: Unimplemented Vulkan API '" << __func__ << "'"         \
            << std::endl;                                                      \
  abort()
