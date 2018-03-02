// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Utils.cpp
/// This file defines miscellaneous utilities used internally by libtalvos.

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace talvos
{

bool checkEnv(const char *Name, bool Default)
{
  const char *Value = getenv(Name);
  if (!Value)
    return Default;

  if (!strcmp(Value, "0"))
    return false;
  else if (!strcmp(Value, "1"))
    return true;

  std::cerr << std::endl
            << "ERROR: Invalid value for " << Name << " environment variable"
            << std::endl;
  abort();
}

} // namespace talvos
