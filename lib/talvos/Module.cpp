// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include "talvos/Module.h"

namespace talvos
{

std::unique_ptr<Module> Module::load(const std::string &FileName)
{
  std::unique_ptr<Module> M(new Module());

  // TODO: Implement

  return M;
}

} // namespace talvos
