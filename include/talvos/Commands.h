// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Commands.h
/// This file declares the Command base class and its subclasses.

#ifndef TALVOS_COMMANDS_H
#define TALVOS_COMMANDS_H

#include "talvos/DescriptorSet.h"
#include "talvos/Dim3.h"

namespace talvos
{

class ComputePipeline;
class Object;

/// This class is a base class for all commands.
class Command
{
public:
  /// Identifies different Command subclasses.
  enum Type
  {
    DISPATCH,
  };

  /// Returns the type of this command.
  Type getType() const { return Ty; }

protected:
  /// Used by subclasses to initialize the command type.
  Command(Type Ty) : Ty(Ty){};

  Type Ty; ///< The type of this command.
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
  DispatchCommand(const ComputePipeline *PL, Dim3 NumGroups,
                  const DescriptorSetMap &DSM)
      : Command(DISPATCH), Pipeline(PL), NumGroups(NumGroups), DSM(DSM)
  {}

  /// Returns the descriptor set map used by the command.
  const DescriptorSetMap &getDescriptorSetMap() const { return DSM; }

  /// Returns the number of workgroups this command launches.
  Dim3 getNumGroups() const { return NumGroups; }

  /// Returns the pipeline this command is invoking.
  const ComputePipeline *getPipeline() const { return Pipeline; }

private:
  const ComputePipeline *Pipeline; ///< The pipeline to use.

  Dim3 NumGroups; ///< The number of workgroups to launch.

  DescriptorSetMap DSM; ///< The descriptor set map to use.
};

} // namespace talvos

#endif
