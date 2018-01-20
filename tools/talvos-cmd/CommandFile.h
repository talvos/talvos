// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <fstream>
#include <memory>

namespace talvos
{
class Device;
class Module;
}

class CommandFile
{
public:
  CommandFile(std::istream &Stream);
  ~CommandFile();
  bool run();

private:
  template <typename T> T get(const char *ParseAction);

  void parseAllocate();
  void parseDescriptorSet();
  void parseDispatch();
  void parseDump();
  void parseEntry();
  void parseModule();

  std::istream &Stream;
  talvos::Device *Device;
  std::unique_ptr<talvos::Module> Module;

  std::string CurrentParseAction;
};
