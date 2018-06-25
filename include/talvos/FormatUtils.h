// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file FormatUtils.h
/// This file declares utility functions for processing VkFormat values.

#ifndef TALVOS_FORMATUTILS_H
#define TALVOS_FORMATUTILS_H

#include "vulkan/vulkan_core.h"

namespace talvos
{

/// Returns the size in bytes for each element of an image with type \p Format.
uint32_t getElementSize(VkFormat Format);

} // namespace talvos

#endif
