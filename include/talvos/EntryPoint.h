// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file EntryPoint.h
/// This file declares the EntryPoint class.

#ifndef TALVOS_ENTRYPOINT_H
#define TALVOS_ENTRYPOINT_H

#include <string>
#include <vector>

namespace talvos
{

class Function;
class Variable;

/// A list of module scope variables.
typedef std::vector<const Variable *> VariableList;

/// This class represents a shader entry point.
class EntryPoint
{
public:
  /// Create an EntryPoint.
  EntryPoint(uint32_t Id, std::string Name, uint32_t ExecutionModel,
             const Function *Func, const VariableList &Variables)
      : Id(Id), Name(Name), ExecutionModel(ExecutionModel), Func(Func),
        Variables(Variables){};

  /// Returns the shader execution model of this entry point.
  uint32_t getExecutionModel() const { return ExecutionModel; }

  /// Returns the function specified by this entry point.
  const Function *getFunction() const { return Func; }

  /// Returns the SPIR-V result ID of this entry point.
  uint32_t getId() const { return Id; }

  /// Returns the name of this entry point.
  const std::string &getName() const { return Name; }

  /// Returns the input/output variables used by this entry point.
  const VariableList &getVariables() const { return Variables; }

private:
  /// The SPIR-V result ID.
  uint32_t Id;

  /// The name of the entry point.
  std::string Name;

  /// The shader execution mode.
  uint32_t ExecutionModel;

  /// The function that will be used.
  const Function *Func;

  /// List of input/output variables used.
  VariableList Variables;
};

} // namespace talvos

#endif
