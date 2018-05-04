// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Commands.h
/// This file declares the Command base class and its subclasses.

#ifndef TALVOS_COMMANDS_H
#define TALVOS_COMMANDS_H

#include <vector>

#include "talvos/DescriptorSet.h"
#include "talvos/Dim3.h"

namespace talvos
{

class Object;
class Pipeline;

/// This class is a base class for all commands.
class Command
{
public:
  Command(){};
  virtual ~Command(){};

  // Do not allow Command objects to be copied.
  ///\{
  Command(const Command &) = delete;
  Command &operator=(const Command &) = delete;
  ///\}
};

/// This class encapsulates information about a compute kernel launch.
class DispatchCommand : public Command
{
public:
  /// List of mappings from SPIR-V result ID to Object for each variable.
  typedef std::vector<std::pair<uint32_t, Object>> VariableList;

  /// Create a new DispatchCommand.
  ///
  /// Any buffers used by \p PL must have a corresponding entry in \p DS.
  ///
  /// \param PL The compute pipeline to invoke.
  /// \param NumGroups The number of groups to launch.
  /// \param DSM The descriptor set mapping to use.
  DispatchCommand(const Pipeline *PL, Dim3 NumGroups,
                  const DescriptorSetMap &DSM);

  /// Return the number of workgroups.
  Dim3 getNumGroups() const { return NumGroups; }

  /// Returns the pipeline this command is invoking.
  const Pipeline *getPipeline() const { return PL; }

  /// Return the resolved buffer variable pointer values for this command.
  const VariableList &getVariables() const { return Variables; }

private:
  const Pipeline *PL; ///< The pipeline to use.

  Dim3 NumGroups; ///< The number of workgroups.

  VariableList Variables; ///< Resolved buffer variable values.
};

} // namespace talvos

#endif
