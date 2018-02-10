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

#include "talvos/Dim3.h"
#include "talvos/Object.h"

namespace talvos
{

class Function;
class Instruction;
class Type;

/// A variable that represents a global buffer object.
struct BufferVariable
{
  const Type *Ty;
  uint32_t DescriptorSet;
  uint32_t Binding;
};

/// A variable that represents a pipeline input.
struct InputVariable
{
  const Type *Ty;
  uint32_t Builtin;
};

/// A variable that represents a private memory allocation.
struct PrivateVariable
{
  const Type *Ty;
  uint32_t Initializer;
};

/// Variable mapping types.
///@{
typedef std::map<uint32_t, BufferVariable> BufferVariableMap;
typedef std::map<uint32_t, InputVariable> InputVariableMap;
typedef std::map<uint32_t, PrivateVariable> PrivateVariableMap;
///@}

/// This class represents a SPIR-V module.
///
/// This class contains types, functions, global variables, and constant
/// instruction results. Factory methods are provided to create a module from a
/// SPIR-V binary filename or from data in memory.
class Module
{
public:
  /// Create an empty module.
  Module(uint32_t IdBound);

  // Destroy the module and all its types, functions, blocks, and instructions.
  ~Module();

  // Do not allow Modules to be copied.
  Module(const Module &) = delete;
  Module &operator=(const Module &) = delete;

  /// Add an entry point with the specified name and function ID.
  void addEntryPoint(std::string Name, uint32_t Id);

  /// Add a function to this module.
  void addFunction(std::unique_ptr<Function> Func);

  /// Add a local size execution mode to an entry point.
  void addLocalSize(uint32_t Entry, Dim3 LocalSize);

  /// Add an object to this module.
  void addObject(uint32_t Id, const Object &Obj);

  /// Add a type to this module.
  void addType(uint32_t Id, std::unique_ptr<Type> Ty);

  /// Add a variable to this module.
  /// Set \p Initializer to 0 to leave the variable uninitialized.
  void addVariable(uint32_t Id, const Type *Ty, uint32_t Initializer);

  /// Get the entry point with the specified name.
  /// Returns nullptr if no entry point called \p Name is found.
  const Function *getEntryPoint(const std::string &Name) const;

  /// Returns the function with the specified ID.
  const Function *getFunction(uint32_t Id) const;

  /// Returns the ID bound of the results in this module.
  uint32_t getIdBound() const { return IdBound; }

  /// Returns the buffer variable map.
  const BufferVariableMap &getBufferVariables() const;

  /// Returns the input variable map.
  const InputVariableMap &getInputVariables() const;

  /// Returns the private variable map.
  const PrivateVariableMap &getPrivateVariables() const;

  /// Returns the LocalSize execution mode for an entry point.
  /// This will return (1,1,1) if it has not been explicitly set for \p Entry.
  Dim3 getLocalSize(uint32_t Entry) const;

  /// Returns the object with the specified ID.
  /// \p Id must be valid constant instruction result.
  const Object &getObject(uint32_t Id) const;

  /// Returns a list of all result objects in this module.
  const std::vector<Object> &getObjects() const;

  /// Returns the type with the specified ID.
  const Type *getType(uint32_t Id) const;

  /// Set the binding decoration of the specified variable ID.
  void setBinding(uint32_t Variable, uint32_t Binding);

  /// Set the builtin decoration for the specified variable ID.
  void setBuiltin(uint32_t Id, uint32_t Builtin);

  /// Set the descriptor set decoration for the specified variable ID.
  void setDescriptorSet(uint32_t Variable, uint32_t DescriptorSet);

  /// Create a new module from the supplied SPIR-V binary data.
  /// Returns nullptr on failure.
  static std::unique_ptr<Module> load(const uint32_t *Words, uint32_t NumWords);

  /// Create a new module from the given SPIR-V binary filename.
  /// Returns nullptr on failure.
  static std::unique_ptr<Module> load(const std::string &FileName);

private:
  typedef std::map<uint32_t, std::unique_ptr<Type>> TypeMap;
  typedef std::map<uint32_t, std::unique_ptr<Function>> FunctionMap;
  typedef std::map<std::string, uint32_t> EntryPointMap;

  uint32_t IdBound;                    ///< The ID bound of the module.
  std::vector<Object> Objects;         ///< Constant instruction results.
  BufferVariableMap BufferVariables;   ///< Buffer variables.
  InputVariableMap InputVariables;     ///< Input variables.
  PrivateVariableMap PrivateVariables; ///< Private variables.
  TypeMap Types;                       ///< Type mapping.
  FunctionMap Functions;               ///< Function mapping.
  EntryPointMap EntryPoints;           ///< Entry point mapping.
  std::map<uint32_t, Dim3> LocalSizes; ///< LocalSize execution modes.
};

} // namespace talvos

#endif
