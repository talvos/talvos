// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <memory>
#include <string>

namespace talvos
{

struct Instruction
{
  uint16_t Opcode;
  uint32_t Type;
  // TODO: Operands
  Instruction *Next;
};

struct Function
{
  // TODO: Name, attributes, etc
  // TODO: Blocks?
  Instruction *FirstInstruction;
};

class Module
{
public:
  void addFunction(Function *Func);
  static std::unique_ptr<Module> load(const std::string &FileName);

private:
  // TODO: Allow multiple functions
  Function *Func;
};

} // namespace talvos
