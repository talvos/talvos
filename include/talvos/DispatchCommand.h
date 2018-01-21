// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_DISPATCHCOMMAND_H
#define TALVOS_DISPATCHCOMMAND_H

#include <map>
#include <vector>

namespace talvos
{

struct Function;
class Device;
class Module;
class Object;

typedef std::map<std::pair<size_t, size_t>, size_t> DescriptorSet;

class DispatchCommand
{
public:
  DispatchCommand(Device *D, const Module *M, const Function *F,
                  const DescriptorSet &DS);
  ~DispatchCommand();
  void run();

private:
  Device *Dev;
  const Module *Mod;
  const Function *Func;
  std::vector<std::pair<uint32_t, Object>> Variables;
};

} // namespace talvos

#endif
