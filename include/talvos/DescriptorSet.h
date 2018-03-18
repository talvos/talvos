// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file DescriptorSet.h
/// This file declares the DescriptorSet type.

#ifndef TALVOS_DESCRIPTORSET_H
#define TALVOS_DESCRIPTORSET_H

#include <map>

namespace talvos
{

/// Map from Descriptor/Binding pair to an address in memory.
typedef std::map<std::pair<uint32_t, uint32_t>, uint64_t> DescriptorSet;

}; // namespace talvos

#endif
