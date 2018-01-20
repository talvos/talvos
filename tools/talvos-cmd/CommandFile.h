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

class CommandFile
{
public:
  CommandFile();
  bool open(const char *FileName);
  ~CommandFile();
  bool run();

private:
  std::ifstream File;
  std::unique_ptr<talvos::Module> Module;
};
