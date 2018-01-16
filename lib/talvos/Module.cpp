// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <cassert>
#include <cstdio>
#include <iostream>
#include <spirv-tools/libspirv.hpp>

#include <spirv/unified1/spirv.h>

#include "talvos/Module.h"

namespace talvos
{

class ModuleBuilder
{
public:
  ModuleBuilder()
  {
    Mod = std::unique_ptr<Module>(new Module);
    CurrentFunction = nullptr;
    PreviousInstruction = nullptr;
  }

  void processInstruction(const spv_parsed_instruction_t *Inst)
  {
    assert(Mod);

    if (Inst->opcode == SpvOpFunction)
    {
      assert(CurrentFunction == nullptr);
      // TODO: Cleanup - when is this destroyed?
      // Should be owned by Module? Module::createFunction()?
      CurrentFunction = new Function;
      CurrentFunction->FirstInstruction = nullptr;
    }
    else if (Inst->opcode == SpvOpFunctionEnd)
    {
      assert(CurrentFunction);
      Mod->addFunction(CurrentFunction);
      CurrentFunction = nullptr;
      PreviousInstruction = nullptr;
    }
    else if (CurrentFunction)
    {
      // TODO: Cleanup - when is this destroyed, by parent Function?
      Instruction *I = new Instruction;
      I->Opcode = Inst->opcode;
      I->NumOperands = Inst->num_operands;
      // TODO: ResultType
      // TODO: Are all operands IDs?
      I->Operands = new uint32_t[I->NumOperands];
      for (int i = 0; i < Inst->num_operands; i++)
      {
        // TODO: Handle larger operands
        assert(Inst->operands[i].num_words == 1);
        I->Operands[i] = Inst->words[Inst->operands[i].offset];
      }
      I->Next = nullptr;
      if (!CurrentFunction->FirstInstruction)
        CurrentFunction->FirstInstruction = I;
      else if (PreviousInstruction)
        PreviousInstruction->Next = I;
      PreviousInstruction = I;
    }
    else
    {
      std::cout << "Unhandled opcode " << Inst->opcode << std::endl;
    }
  };

  std::unique_ptr<Module> takeModule()
  {
    assert(Mod);
    return std::move(Mod);
  }

private:
  std::unique_ptr<Module> Mod;
  Function *CurrentFunction;
  Instruction *PreviousInstruction;
};

spv_result_t HandleHeader(void *user_data, spv_endianness_t endian,
                          uint32_t /* magic */, uint32_t version,
                          uint32_t generator, uint32_t id_bound,
                          uint32_t schema)
{
  // TODO: Do something with id_bound
  std::cout << "Module ID bound = " << id_bound << std::endl;
  return SPV_SUCCESS;
}

spv_result_t
HandleInstruction(void *user_data,
                  const spv_parsed_instruction_t *parsed_instruction)
{
  ((ModuleBuilder *)user_data)->processInstruction(parsed_instruction);
  return SPV_SUCCESS;
}

void Module::addFunction(Function *Func)
{
  // TODO: Support multiple functions
  this->Func = Func;
}

Function* Module::getFunction() const
{
  // TODO: Support multiple functions
  return this->Func;
}

std::unique_ptr<Module> Module::load(const std::string &FileName)
{
  // Open file.
  FILE *SPVFile = fopen(FileName.c_str(), "rb");
  if (!SPVFile)
  {
    std::cerr << "Failed to open '" << FileName << "'" << std::endl;
    return nullptr;
  }

  // Read file data.
  fseek(SPVFile, 0, SEEK_END);
  long NumBytes = ftell(SPVFile);
  uint32_t Words[NumBytes];
  fseek(SPVFile, 0, SEEK_SET);
  fread(Words, 1, NumBytes, SPVFile);
  fclose(SPVFile);

  ModuleBuilder MB;

  // Parse binary.
  spv_diagnostic diagnostic = nullptr;
  spvtools::Context SPVContext(SPV_ENV_UNIVERSAL_1_2);
  spvBinaryParse(SPVContext.CContext(), &MB, Words, NumBytes / 4, HandleHeader,
                 HandleInstruction, &diagnostic);
  if (diagnostic)
  {
    spvDiagnosticPrint(diagnostic);
    return nullptr;
  }

  return MB.takeModule();
}

} // namespace talvos
