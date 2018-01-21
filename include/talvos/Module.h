// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_MODULE_H
#define TALVOS_MODULE_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "talvos/Object.h"

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

struct BufferVariable
{
  // TODO: Type
  uint32_t StorageClass;
  // TODO: Initializer

  uint32_t DescriptorSet;
  uint32_t Binding;
};
typedef std::map<uint32_t, BufferVariable> BufferVariableMap;
typedef std::map<uint32_t, uint32_t> InputVariableMap;

class Module
{
public:
  Module(uint32_t IdBound);
  ~Module();
  void addFunction(Function *Func);
  void addObject(uint32_t Id, const Object &Obj);
  void addVariable(uint32_t Id, uint32_t StorageClass);
  std::vector<Object> cloneObjects() const;
  Function *getFunction() const;
  uint32_t getIdBound() const { return IdBound; }
  const BufferVariableMap &getBufferVariables() const;
  const InputVariableMap &getInputVariables() const;
  static std::unique_ptr<Module> load(const std::string &FileName);

  void setBinding(uint32_t Variable, uint32_t Binding);
  void setBuiltin(uint32_t Id, uint32_t Builtin);
  void setDescriptorSet(uint32_t Variable, uint32_t DescriptorSet);

private:
  // TODO: Allow multiple functions
  Function *Func;
  uint32_t IdBound;
  std::vector<Object> Objects;
  BufferVariableMap BufferVariables;
  InputVariableMap InputVariables;
};

} // namespace talvos

#endif
