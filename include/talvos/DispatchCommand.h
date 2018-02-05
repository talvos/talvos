// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_DISPATCHCOMMAND_H
#define TALVOS_DISPATCHCOMMAND_H

#include <map>
#include <vector>

namespace talvos
{

class Device;
class Function;
class Module;
class Object;

/// Map from Descriptor/Binding pair to an address in memory.
typedef std::map<std::pair<size_t, size_t>, size_t> DescriptorSet;

/// This class is used to invoke compute kernels on a virtual device.
class DispatchCommand
{
public:
  /// Create a new DispatchCommand.
  ///
  /// The function \p F must belong to module \p M. Any buffers used by \p F
  /// must have a corresponding entry in \p DS.
  ///
  /// \param D The target device.
  /// \param M The module containing the entry point to invoke.
  /// \param F The entry point to invoke.
  /// \param GroupCountX The number of groups in the X dimension.
  /// \param GroupCountY The number of groups in the Y dimension.
  /// \param GroupCountZ The number of groups in the Z dimension.
  /// \param DS The descriptor set mapping to use.
  DispatchCommand(Device *D, const Module *M, const Function *F,
                  uint32_t GroupCountX, uint32_t GroupCountY,
                  uint32_t GroupCountZ, const DescriptorSet &DS);

  DispatchCommand(const DispatchCommand &) = delete;
  DispatchCommand &operator=(const DispatchCommand &) = delete;

  /// Run the dispatch command to completion.
  void run();

private:
  Device *Dev;          ///< The target device.
  const Module *Mod;    ///< The module containing the entry point to invoke.
  const Function *Func; ///< The entry point to invoke.
  uint32_t GroupCountX; ///< The number of groups in the X dimension.
  uint32_t GroupCountY; ///< The number of groups in the Y dimension.
  uint32_t GroupCountZ; ///< The number of groups in the Z dimension.
  std::vector<std::pair<uint32_t, Object>>
      Variables;        ///< Resolved variable values.
};

} // namespace talvos

#endif
