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
  /// Create a new DispatchCommand.
  ///
  /// Any buffers used by \p PL must have a corresponding entry in \p DSM.
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

  /// Returns the initial object values for this command.
  const std::vector<Object> &getObjects() const { return Objects; };

private:
  const Pipeline *PL; ///< The pipeline to use.

  Dim3 NumGroups; ///< The number of workgroups.

  std::vector<Object> Objects; ///< Initial object values.
};

} // namespace talvos

#endif
