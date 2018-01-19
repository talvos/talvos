// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_DISPATCHCOMMAND_H
#define TALVOS_DISPATCHCOMMAND_H

namespace talvos
{

struct Function;
class Module;

class DispatchCommand
{
public:
  DispatchCommand(const Module *M, const Function *F);
  void run();

private:
  const Module *Mod;
  const Function *Func;
};

} // namespace talvos

#endif
