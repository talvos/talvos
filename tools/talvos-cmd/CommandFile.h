// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <fstream>
#include <map>
#include <memory>

#include "talvos/DispatchCommand.h"

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

  template <typename T> void dump();
  template <typename T> void data(uint64_t Address, uint64_t NumBytes);
  template <typename T> void fill(uint64_t Address, uint64_t NumBytes);
  template <typename T> void range(uint64_t Address, uint64_t NumBytes);

  std::istream &Stream;
  talvos::Device *Device;
  std::unique_ptr<talvos::Module> Module;
  const talvos::Function *Function;
  std::map<std::string, std::pair<uint64_t, uint64_t>> Buffers;
  talvos::DescriptorSet DescriptorSet;

  std::string CurrentParseAction;
};
