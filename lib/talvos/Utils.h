// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Utils.h
/// This file declares miscellaneous utilities used internally by libtalvos.

#ifndef TALVOS_UTILS_H
#define TALVOS_UTILS_H

namespace talvos
{

/// Returns true if the environment variable \p Name is set to 1, false for 0,
/// or \p Default if it is not set.
bool checkEnv(const char *Name, bool Default);

} // namespace talvos

#endif
