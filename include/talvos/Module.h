// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_MODULE_H
#define TALVOS_MODULE_H

#include <memory>
#include <string>
#include <vector>

#include "talvos/Result.h"

namespace talvos
{

struct Instruction
{
  uint16_t Opcode;
  uint16_t NumOperands;
  // TODO: Currently assumes all operands are 32-bit IDs
  uint32_t *Operands;
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
  Module(uint32_t IdBound);
  ~Module();
  void addFunction(Function *Func);
  void addResult(uint32_t Id, const Result &R);
  std::vector<Result> cloneResults() const;
  Function *getFunction() const;
  uint32_t getIdBound() const { return IdBound; }
  static std::unique_ptr<Module> load(const std::string &FileName);

private:
  // TODO: Allow multiple functions
  Function *Func;
  uint32_t IdBound;
  std::vector<Result> Results;
};

} // namespace talvos

#endif
