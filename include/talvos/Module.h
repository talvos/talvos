// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Module.h
/// This file declares the Module class.

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
class Variable;

/// A list of module scope variables.
typedef std::vector<const Variable *> VariableList;

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

  // Do not allow Module objects to be copied.
  ///\{
  Module(const Module &) = delete;
  Module &operator=(const Module &) = delete;
  ///\}

  /// Add an entry point with the specified name, execution model and result ID.
  void addEntryPoint(std::string Name, uint32_t ExecutionModel, uint32_t Id);

  /// Add a function to this module.
  void addFunction(std::unique_ptr<Function> Func);

  /// Add a local size execution mode to an entry point.
  void addLocalSize(uint32_t Entry, Dim3 LocalSize);

  /// Add an object to this module.
  void addObject(uint32_t Id, const Object &Obj);

  /// Add a specialization constant ID mapping.
  void addSpecConstant(uint32_t SpecId, uint32_t ResultId);

  /// Add a specialization constant operation instruction to this module.
  /// Transfers ownership of \p Op to the module.
  void addSpecConstantOp(Instruction *Op);

  /// Add a type to this module.
  void addType(uint32_t Id, std::unique_ptr<Type> Ty);

  /// Add a variable to this module, transferring ownership to the module.
  void addVariable(Variable *Var) { Variables.push_back(Var); }

  /// Get the entry point with the specified name and SPIR-V execution model.
  /// Returns nullptr if no entry point called \p Name with a matching execution
  /// model is found.
  const Function *getEntryPoint(const std::string &Name,
                                uint32_t ExecutionModel) const;

  /// Get the entry point name for the specified function ID.
  /// Returns an empty string if no entry point matching \p Id is found.
  std::string getEntryPointName(uint32_t Id) const;

  /// Returns the function with the specified ID.
  const Function *getFunction(uint32_t Id) const;

  /// Returns the ID bound of the results in this module.
  uint32_t getIdBound() const { return IdBound; }

  /// Returns the LocalSize execution mode for an entry point.
  /// This will return (1,1,1) if it has not been explicitly set for \p Entry.
  Dim3 getLocalSize(uint32_t Entry) const;

  /// Returns the object with the specified ID.
  /// \p Id must be valid constant instruction result.
  const Object &getObject(uint32_t Id) const;

  /// Returns a list of all result objects in this module.
  const std::vector<Object> &getObjects() const;

  /// Returns the result ID for the given specialization constant ID.
  /// Returns 0 if no specialization constants with this ID are present.
  uint32_t getSpecConstant(uint32_t SpecId) const;

  /// Returns the list of specialization constant operation instructions.
  const std::vector<Instruction *> &getSpecConstantOps() const;

  /// Returns the type with the specified ID.
  const Type *getType(uint32_t Id) const;

  /// Returns the list of module scope variables.
  const VariableList &getVariables() const { return Variables; }

  /// Returns the ID of the object decorated with WorkgroupSize.
  /// Returns 0 if no object has been decorated with WorkgroupSize.
  uint32_t getWorkgroupSizeId() const { return WorkgroupSizeId; }

  /// Set the ID of the object decorated with WorkgroupSize.
  void setWorkgroupSizeId(uint32_t Id) { WorkgroupSizeId = Id; }

  /// Create a new module from the supplied SPIR-V binary data.
  /// Returns nullptr on failure.
  static std::unique_ptr<Module> load(const uint32_t *Words, size_t NumWords);

  /// Create a new module from the given SPIR-V binary filename.
  /// Returns nullptr on failure.
  static std::unique_ptr<Module> load(const std::string &FileName);

private:
  /// Map from SPIR-V result ID to talvos::Type.
  typedef std::map<uint32_t, std::unique_ptr<Type>> TypeMap;

  /// Map from SPIR-V result ID to talvos::Function.
  typedef std::map<uint32_t, std::unique_ptr<Function>> FunctionMap;

  /// Map entry point name and execution model to SPIR-V function result ID.
  typedef std::map<std::pair<std::string, uint32_t>, uint32_t> EntryPointMap;

  uint32_t IdBound;                    ///< The ID bound of the module.
  std::vector<Object> Objects;         ///< Constant instruction results.
  TypeMap Types;                       ///< Type mapping.
  FunctionMap Functions;               ///< Function mapping.
  EntryPointMap EntryPoints;           ///< Entry point mapping.
  std::map<uint32_t, Dim3> LocalSizes; ///< LocalSize execution modes.

  /// Map specialization constant IDs to result IDs.
  std::map<uint32_t, uint32_t> SpecConstants;

  /// List of specialization constant operation instructions.
  std::vector<Instruction *> SpecConstantOps;

  /// The ID of the object decorated with WorkgroupSize.
  uint32_t WorkgroupSizeId;

  /// Module scope variables.
  VariableList Variables;
};

} // namespace talvos

#endif
