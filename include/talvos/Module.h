// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <memory>
#include <string>

namespace talvos
{

class Module
{
public:
  static std::unique_ptr<Module> load(const std::string &FileName);
};

} // namespace talvos
