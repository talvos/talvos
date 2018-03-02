// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_FUNCTION_H
#define TALVOS_FUNCTION_H

#include <map>
#include <memory>
#include <vector>

namespace talvos
{

class Block;
class Type;

/// This class represents a function in a SPIR-V Module.
class Function
{
public:
  /// Create a new function with an ID and a type.
  Function(uint32_t Id, const Type *FunctionType);

  // Do not allow Function objects to be copied.
  ///\{
  Function(const Function &) = delete;
  Function &operator=(const Function &) = delete;
  ///\}

  /// Add a block to this function.
  void addBlock(std::unique_ptr<Block> B);

  /// Add a parameter to this function.
  void addParam(uint32_t Id) { Parameters.push_back(Id); }

  /// Returns the block with ID \p Id.
  const Block *getBlock(uint32_t Id) const { return Blocks.at(Id).get(); }

  /// Returns the first block in this function.
  const Block *getFirstBlock() const { return Blocks.at(FirstBlockId).get(); }

  /// Returns the ID of the first block in this function.
  uint32_t getFirstBlockId() const { return FirstBlockId; }

  /// Returns the ID of this function.
  uint32_t getId() const { return Id; }

  /// Returns the ID of the parameter at index \p I.
  uint32_t getParamId(uint32_t I) const { return Parameters[I]; }

  /// Returns the number of parameters in this function.
  uint32_t getNumParams() const { return Parameters.size(); }

  /// Sets the ID of the entry block in this function.
  void setFirstBlock(uint32_t Id) { FirstBlockId = Id; }

private:
  /// A mapping from IDs to Blocks.
  typedef std::map<uint32_t, std::unique_ptr<Block>> BlockMap;

  uint32_t Id;              ///< The ID of this function.
  const Type *FunctionType; ///< The function type.
  uint32_t FirstBlockId;    ///< The ID of the first block.
  BlockMap Blocks;          ///< The blocks in the function.

  std::vector<uint32_t> Parameters; ///< The function parameter IDs.
};

} // namespace talvos

#endif
