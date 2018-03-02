// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Device.cpp
/// This file defines the Device class.

#include "talvos/Device.h"
#include "talvos/Memory.h"

namespace talvos
{

Device::Device() { GlobalMemory = new Memory; }

Device::~Device() { delete GlobalMemory; }

} // namespace talvos
