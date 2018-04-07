// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Pipeline.h
/// This file declares the Pipeline class.

#ifndef TALVOS_PIPELINE_H
#define TALVOS_PIPELINE_H

#include <map>
#include <vector>

#include "talvos/Dim3.h"

namespace talvos
{

class Device;
class Function;
class Module;
class Object;

/// Mapping from specialization constant ID to Object values.
typedef std::map<uint32_t, Object> SpecConstantMap;

/// This class encapsulates information about a pipeline.
class Pipeline
{
public:
  /// Create a new Pipeline.
  ///
  /// The function \p F must belong to module \p M.
  ///
  /// \param D The device on which to create the pipeline.
  /// \param M The module containing the entry point to invoke.
  /// \param F The entry point to invoke.
  /// \param SM A map of specialization constant values.
  Pipeline(Device &D, const Module *M, const Function *F,
           const SpecConstantMap &SM = {});

  // Do not allow Pipeline objects to be copied.
  ///\{
  Pipeline(const Pipeline &) = delete;
  Pipeline &operator=(const Pipeline &) = delete;
  ///\}

  /// Return the function this pipeline will invoke.
  const Function *getFunction() const { return Func; }

  /// Return the workgroup size.
  Dim3 getGroupSize() const { return GroupSize; }

  /// Return the module this pipeline is using.
  const Module *getModule() const { return Mod; }

  /// Returns a list of all result objects in this pipeline.
  const std::vector<Object> &getObjects() const { return Objects; };

private:
  const Module *Mod;    ///< The module containing the entry point to invoke.
  const Function *Func; ///< The entry point to invoke.
  Dim3 GroupSize;       ///< The size of each workgroup.

  /// The result objects in this pipeline, after specialization.
  std::vector<Object> Objects;
};

} // namespace talvos

#endif
