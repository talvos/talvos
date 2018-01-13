// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <fstream>
#include <memory>

namespace talvos
{
class Module;
}

class CommandInvocation
{
public:
  CommandInvocation();
  ~CommandInvocation();
  bool load(const char *FileName);
  void run();

private:
  std::ifstream ConfigFile;
  std::unique_ptr<talvos::Module> Module;
};
