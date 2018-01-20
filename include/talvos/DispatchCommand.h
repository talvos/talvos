// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_DISPATCHCOMMAND_H
#define TALVOS_DISPATCHCOMMAND_H

#include <map>

namespace talvos
{

struct Function;
class Device;
class Module;

typedef std::map<std::pair<size_t, size_t>, size_t> DescriptorSet;

class DispatchCommand
{
public:
  DispatchCommand(Device *D, const Module *M, const Function *F,
                  const DescriptorSet &DS);
  void run();

private:
  Device *Dev;
  const Module *Mod;
  const Function *Func;
};

} // namespace talvos

#endif
