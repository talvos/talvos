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

struct Instruction;
class Function;
class Type;

struct BufferVariable
{
  const Type *Ty;
  uint32_t DescriptorSet;
  uint32_t Binding;
};

struct InputVariable
{
  const Type *Ty;
  uint32_t Builtin;
};

struct PrivateVariable
{
  const Type *Ty;
  uint32_t Initializer;
};

typedef std::map<uint32_t, BufferVariable> BufferVariableMap;
typedef std::map<uint32_t, InputVariable> InputVariableMap;
typedef std::map<uint32_t, PrivateVariable> PrivateVariableMap;

class Module
{
public:
  Module(uint32_t IdBound);
  ~Module();
  void addEntryPoint(std::string Name, uint32_t Id);
  void addFunction(std::unique_ptr<Function> Func);
  void addObject(uint32_t Id, const Object &Obj);
  void addType(uint32_t Id, Type *T);
  void addVariable(uint32_t Id, const Type *Ty, uint32_t Initializer);
  std::vector<Object> cloneObjects() const;
  const Function *getEntryPoint(const std::string &Name) const;
  const Function *getFunction(uint32_t Id) const;
  uint32_t getIdBound() const { return IdBound; }
  const BufferVariableMap &getBufferVariables() const;
  const InputVariableMap &getInputVariables() const;
  const PrivateVariableMap &getPrivateVariables() const;
  const Object &getObject(uint32_t Id) const;
  const Type *getType(uint32_t Id) const;

  static std::unique_ptr<Module> load(const uint32_t *Words, uint32_t NumWords);
  static std::unique_ptr<Module> load(const std::string &FileName);

  void setBinding(uint32_t Variable, uint32_t Binding);
  void setBuiltin(uint32_t Id, uint32_t Builtin);
  void setDescriptorSet(uint32_t Variable, uint32_t DescriptorSet);

private:
  uint32_t IdBound;
  std::vector<Object> Objects;
  BufferVariableMap BufferVariables;
  InputVariableMap InputVariables;
  PrivateVariableMap PrivateVariables;
  std::map<uint32_t, Type *> Types;
  std::map<uint32_t, std::unique_ptr<Function>> Functions;
  std::map<std::string, uint32_t> EntryPoints;
};

} // namespace talvos

#endif
