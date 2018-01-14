// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <cstdio>
#include <iostream>
#include <spirv-tools/libspirv.hpp>

#include "talvos/Module.h"

namespace talvos
{

spv_result_t ParseHeader(void *user_data, spv_endianness_t endian,
                         uint32_t /* magic */, uint32_t version,
                         uint32_t generator, uint32_t id_bound, uint32_t schema)
{
  // TODO: Do something with id_bound
  std::cout << "Module ID bound = " << id_bound << std::endl;
  return SPV_SUCCESS;
}

spv_result_t
ParseInstruction(void *user_data,
                 const spv_parsed_instruction_t *parsed_instruction)
{
  // TODO: Implement
  std::cout << "Opcode " << parsed_instruction->opcode << std::endl;
  return SPV_SUCCESS;
}

std::unique_ptr<Module> Module::load(const std::string &FileName)
{
  std::unique_ptr<Module> M(new Module());

  // Open file
  FILE *SPVFile = fopen(FileName.c_str(), "rb");
  if (!SPVFile)
  {
    std::cerr << "Failed to open '" << FileName << "'" << std::endl;
    return nullptr;
  }

  // Read file data
  fseek(SPVFile, 0, SEEK_END);
  long NumBytes = ftell(SPVFile);
  uint32_t Words[NumBytes];
  fseek(SPVFile, 0, SEEK_SET);
  fread(Words, 1, NumBytes, SPVFile);
  fclose(SPVFile);

  // Parse binary
  spv_diagnostic diagnostic = nullptr;
  spvtools::Context SPVContext(SPV_ENV_UNIVERSAL_1_2);
  spvBinaryParse(SPVContext.CContext(), M.get(), Words, NumBytes / 4,
                 ParseHeader, ParseInstruction, &diagnostic);
  if (diagnostic)
  {
    spvDiagnosticPrint(diagnostic);
    return nullptr;
  }

  return M;
}

} // namespace talvos
