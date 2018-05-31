// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <fstream>
#include <map>
#include <memory>
#include <vector>

#include "talvos/DescriptorSet.h"
#include "talvos/PipelineStage.h"

namespace talvos
{
class Device;
class EntryPoint;
class Module;
} // namespace talvos

class CommandFile
{
public:
  CommandFile(std::istream &Stream);
  ~CommandFile();
  bool run();

private:
  template <typename T> T get(const char *ParseAction);

  void parseBuffer();
  void parseDescriptorSet();
  void parseDispatch();
  void parseDump();
  void parseEndLoop();
  void parseEntry();
  void parseLoop();
  void parseModule();
  void parseSpecialize();

  template <typename T> void dump(unsigned VecWidth);
  template <typename T> void data(uint64_t Address, uint64_t NumBytes);
  template <typename T> void fill(uint64_t Address, uint64_t NumBytes);
  template <typename T> void series(uint64_t Address, uint64_t NumBytes);
  template <typename T> void specialize(uint32_t SpecId);

  std::istream &Stream;
  talvos::Device *Device;
  std::shared_ptr<talvos::Module> Module;
  const talvos::EntryPoint *Entry;
  std::map<std::string, std::pair<uint64_t, uint64_t>> Buffers;
  talvos::SpecConstantMap SpecConstMap;
  talvos::DescriptorSetMap DescriptorSets;
  std::vector<std::pair<size_t, std::streampos>> Loops;

  size_t CurrentLine;
  std::string CurrentParseAction;
};
