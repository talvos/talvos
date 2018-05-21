// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file PipelineStage.h
/// This file declares the PipelineStage class.

#ifndef TALVOS_PIPELINESTAGE_H
#define TALVOS_PIPELINESTAGE_H

#include <map>
#include <vector>

#include "talvos/Dim3.h"
#include "talvos/Object.h"

namespace talvos
{

class Device;
class EntryPoint;
class Module;

/// Mapping from specialization constant ID to Object values.
typedef std::map<uint32_t, Object> SpecConstantMap;

/// This class encapsulates information about a pipeline stage.
class PipelineStage
{
public:
  /// Create a new PipelineStage.
  ///
  /// The function \p F must belong to module \p M.
  ///
  /// \param D The device on which to create the pipeline stage.
  /// \param M The module containing the entry point to invoke.
  /// \param EP The entry point to invoke.
  /// \param SM A map of specialization constant values.
  PipelineStage(Device &D, const Module *M, const EntryPoint *EP,
                const SpecConstantMap &SM = {});

  // Do not allow PipelineStage objects to be copied.
  ///\{
  PipelineStage(const PipelineStage &) = delete;
  PipelineStage &operator=(const PipelineStage &) = delete;
  ///\}

  /// Return the entry point this pipeline stage will invoke.
  const EntryPoint *getEntryPoint() const { return EP; }

  /// Return the workgroup size.
  Dim3 getGroupSize() const { return GroupSize; }

  /// Return the module this pipeline stage is using.
  const Module *getModule() const { return Mod; }

  /// Returns a list of all result objects in this pipeline stage.
  const std::vector<Object> &getObjects() const { return Objects; };

private:
  const Module *Mod;    ///< The module containing the entry point to invoke.
  const EntryPoint *EP; ///< The entry point to invoke.
  Dim3 GroupSize;       ///< The size of each workgroup.

  /// The result objects in this pipeline stage, after specialization.
  std::vector<Object> Objects;
};

} // namespace talvos

#endif
