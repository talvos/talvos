// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <cassert>
#include <cstdio>
#include <iostream>
#include <spirv-tools/libspirv.hpp>

#include <spirv/unified1/spirv.h>

#include "talvos/Function.h"
#include "talvos/Instruction.h"
#include "talvos/Module.h"
#include "talvos/Type.h"

namespace talvos
{

class ModuleBuilder
{
public:
  void init(uint32_t IdBound)
  {
    assert(!Mod && "Module already initialized");
    Mod = std::unique_ptr<Module>(new Module(IdBound));
    CurrentFunction = nullptr;
    CurrentBlock = nullptr;
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
      const Type *FuncType =
          Mod->getType(Inst->words[Inst->operands[3].offset]);
      CurrentFunction = std::make_unique<Function>(Inst->result_id, FuncType);
    }
    else if (Inst->opcode == SpvOpFunctionEnd)
    {
      assert(CurrentFunction);
      assert(CurrentBlock);
      CurrentFunction->addBlock(CurrentBlock);
      Mod->addFunction(std::move(CurrentFunction));
      CurrentFunction = nullptr;
      CurrentBlock = nullptr;
    }
    else if (CurrentFunction)
    {
      if (Inst->opcode == SpvOpLabel)
      {
        if (CurrentBlock)
          // Add previous block to function.
          CurrentFunction->addBlock(CurrentBlock);
        else
          // First block - set as entry block.
          CurrentFunction->setEntryBlock(Inst->result_id);

        // Create new block.
        CurrentBlock = new Block;
        CurrentBlock->Id = Inst->result_id;
        CurrentBlock->FirstInstruction = nullptr;
        PreviousInstruction = nullptr;
      }
      else
      {
        // TODO: Cleanup - when is this destroyed, by parent Function?
        Instruction *I = new Instruction;
        I->ResultType = Inst->type_id ? Mod->getType(Inst->type_id) : nullptr;
        I->Opcode = Inst->opcode;
        I->NumOperands = Inst->num_operands;
        // TODO: Are all operands IDs?
        I->Operands = new uint32_t[I->NumOperands];
        for (int i = 0; i < Inst->num_operands; i++)
        {
          // TODO: Handle larger operands
          assert(Inst->operands[i].num_words == 1);
          I->Operands[i] = Inst->words[Inst->operands[i].offset];
        }
        I->Next = nullptr;
        if (!CurrentBlock->FirstInstruction)
          CurrentBlock->FirstInstruction = I;
        else if (PreviousInstruction)
          PreviousInstruction->Next = I;
        PreviousInstruction = I;
      }
    }
    else
    {
      switch (Inst->opcode)
      {
      case SpvOpCapability:
      {
        uint32_t Capbility = Inst->words[Inst->operands[0].offset];
        switch (Capbility)
        {
        case SpvCapabilityInt16:
        case SpvCapabilityInt64:
        case SpvCapabilityShader:
        case SpvCapabilityVariablePointers:
        case SpvCapabilityVariablePointersStorageBuffer:
          break;
        default:
          std::cerr << "WARNING: Unrecognized capability " << Capbility
                    << std::endl;
        }
        break;
      }
      case SpvOpConstant:
      case SpvOpSpecConstant: // TODO: Handle specialization constants
      {
        const Type *Ty = Mod->getType(Inst->type_id);
        // TODO: Use actual type
        uint32_t Value = Inst->words[Inst->operands[2].offset];
        Mod->addObject(Inst->result_id, Object::create<uint32_t>(Ty, Value));
        break;
      }
      case SpvOpConstantComposite:
      case SpvOpSpecConstantComposite: // TODO: Handle specialization constants
      {
        const Type *Ty = Mod->getType(Inst->type_id);

        // Build list of constituent values.
        std::vector<Object> Constituents;
        for (int i = 2; i < Inst->num_operands; i++)
        {
          uint32_t Id = Inst->words[Inst->operands[i].offset];
          Constituents.push_back(Mod->getObject(Id));
        }

        // Create and add composite.
        Mod->addObject(Inst->result_id,
                       Object::createComposite(Ty, Constituents));
        break;
      }
      case SpvOpDecorate:
      {
        uint32_t Target = Inst->words[Inst->operands[0].offset];
        uint32_t Decoration = Inst->words[Inst->operands[1].offset];
        switch (Decoration)
        {
        case SpvDecorationArrayStride:
          // TODO: Probably need to handle this?
          break;
        case SpvDecorationBinding:
          Mod->setBinding(Target, Inst->words[Inst->operands[2].offset]);
          break;
        case SpvDecorationBlock:
          // TODO: Need to handle this?
          break;
        case SpvDecorationDescriptorSet:
          Mod->setDescriptorSet(Target, Inst->words[Inst->operands[2].offset]);
          break;
        case SpvDecorationBuiltIn:
          Mod->setBuiltin(Target, Inst->words[Inst->operands[2].offset]);
          break;
        case SpvDecorationSpecId:
          // TODO: Handle this
          break;
        default:
          std::cout << "Unhandled decoration " << Decoration << std::endl;
        }
        break;
      }
      case SpvOpEntryPoint:
      {
        uint32_t ExecutionModel = Inst->words[Inst->operands[0].offset];
        uint32_t Id = Inst->words[Inst->operands[1].offset];
        char *Name = (char *)(Inst->words + Inst->operands[2].offset);
        if (ExecutionModel != SpvExecutionModelGLCompute)
        {
          std::cerr << "WARNING: Unrecognized execution model "
                    << ExecutionModel << std::endl;
        }
        Mod->addEntryPoint(Name, Id);
        break;
      }
      case SpvOpExtension:
      {
        char *Extension = (char *)(Inst->words + Inst->operands[0].offset);
        if (strcmp(Extension, "SPV_KHR_storage_buffer_storage_class") &&
            strcmp(Extension, "SPV_KHR_variable_pointers"))
        {
          std::cerr << "WARNING: Unrecognized extension " << Extension
                    << std::endl;
        }
        break;
      }
      case SpvOpMemberDecorate:
      {
        uint32_t Target = Inst->words[Inst->operands[0].offset];
        uint32_t Member = Inst->words[Inst->operands[1].offset];
        uint32_t Decoration = Inst->words[Inst->operands[2].offset];
        uint32_t Offset = Inst->operands[3].offset;
        switch (Decoration)
        {
        case SpvDecorationOffset:
          MemberOffsets[{Target, Member}] = Inst->words[Offset];
          break;
        default:
          std::cout << "Unhandled decoration " << Decoration << std::endl;
        }
        break;
      }
      case SpvOpMemoryModel:
      {
        uint32_t AddressingMode = Inst->words[Inst->operands[0].offset];
        uint32_t MemoryModel = Inst->words[Inst->operands[1].offset];
        if (AddressingMode != SpvAddressingModelLogical)
        {
          std::cerr << "WARNING: Unrecognized addressing mode "
                    << AddressingMode << std::endl;
        }
        if (MemoryModel != SpvMemoryModelGLSL450)
        {
          std::cerr << "WARNING: Unrecognized memory model " << MemoryModel
                    << std::endl;
        }
        break;
      }
      case SpvOpSource:
        // TODO: Do something with this
        break;
      case SpvOpTypeArray:
      {
        const Type *ElemType =
            Mod->getType(Inst->words[Inst->operands[1].offset]);
        uint32_t Length = Inst->words[Inst->operands[2].offset];
        Mod->addType(Inst->result_id, Type::getArray(ElemType, Length));
        break;
      }
      case SpvOpTypeBool:
      {
        Mod->addType(Inst->result_id, Type::getBool());
        break;
      }
      case SpvOpTypeInt:
      {
        uint32_t Width = Inst->words[Inst->operands[1].offset];
        Mod->addType(Inst->result_id, Type::getInt(Width));
        break;
      }
      case SpvOpTypeFunction:
      {
        uint32_t ReturnType = Inst->words[Inst->operands[1].offset];
        std::vector<const Type *> ArgTypes;
        for (int i = 3; i < Inst->num_operands; i++)
        {
          uint32_t ArgType = Inst->words[Inst->operands[i].offset];
          ArgTypes.push_back(Mod->getType(ArgType));
        }
        Mod->addType(Inst->result_id,
                     Type::getFunction(Mod->getType(ReturnType), ArgTypes));
        break;
      }
      case SpvOpTypePointer:
      {
        uint32_t StorageClass = Inst->words[Inst->operands[1].offset];
        const Type *ElemType =
            Mod->getType(Inst->words[Inst->operands[2].offset]);
        Mod->addType(Inst->result_id, Type::getPointer(StorageClass, ElemType));
        break;
      }
      case SpvOpTypeRuntimeArray:
      {
        const Type *ElemType =
            Mod->getType(Inst->words[Inst->operands[1].offset]);
        Mod->addType(Inst->result_id, Type::getRuntimeArray(ElemType));
        break;
      }
      case SpvOpTypeStruct:
      {
        StructElementTypeList ElemTypes;
        for (int i = 1; i < Inst->num_operands; i++)
        {
          uint32_t ElemType = Inst->words[Inst->operands[i].offset];
          uint32_t ElemOffset = MemberOffsets.at({Inst->result_id, i - 1});
          ElemTypes.push_back({Mod->getType(ElemType), ElemOffset});
        }
        Mod->addType(Inst->result_id, Type::getStruct(ElemTypes));
        break;
      }
      case SpvOpTypeVector:
      {
        const Type *ElemType =
            Mod->getType(Inst->words[Inst->operands[1].offset]);
        uint32_t ElemCount = Inst->words[Inst->operands[2].offset];
        Mod->addType(Inst->result_id, Type::getVector(ElemType, ElemCount));
        break;
      }
      case SpvOpTypeVoid:
      {
        Mod->addType(Inst->result_id, Type::getVoid());
        break;
      }
      case SpvOpVariable:
        Mod->addVariable(Inst->result_id, Mod->getType(Inst->type_id),
                         ((Inst->num_operands > 3)
                              ? Inst->words[Inst->operands[3].offset]
                              : 0));
        break;
      default:
        std::cout << "Unhandled opcode " << Inst->opcode << std::endl;
      }
    }
  };

  std::unique_ptr<Module> takeModule()
  {
    assert(Mod);
    return std::move(Mod);
  }

private:
  std::unique_ptr<Module> Mod;
  std::unique_ptr<Function> CurrentFunction;
  Block *CurrentBlock;
  Instruction *PreviousInstruction;
  std::map<std::pair<uint32_t, uint32_t>, uint32_t> MemberOffsets;
};

spv_result_t HandleHeader(void *user_data, spv_endianness_t endian,
                          uint32_t /* magic */, uint32_t version,
                          uint32_t generator, uint32_t id_bound,
                          uint32_t schema)
{
  ((ModuleBuilder *)user_data)->init(id_bound);
  return SPV_SUCCESS;
}

spv_result_t
HandleInstruction(void *user_data,
                  const spv_parsed_instruction_t *parsed_instruction)
{
  ((ModuleBuilder *)user_data)->processInstruction(parsed_instruction);
  return SPV_SUCCESS;
}

Module::Module(uint32_t IdBound)
{
  this->IdBound = IdBound;
  this->Objects.resize(IdBound);
}

Module::~Module()
{
  for (Object &Obj : Objects)
    Obj.destroy();
}

void Module::addEntryPoint(std::string Name, uint32_t Id)
{
  assert(EntryPoints.count(Name) == 0);
  EntryPoints[Name] = Id;
}

void Module::addFunction(std::unique_ptr<Function> Func)
{
  assert(Functions.count(Func->getId()) == 0);
  Functions[Func->getId()] = std::move(Func);
}

void Module::addObject(uint32_t Id, const Object &Obj)
{
  assert(Id < Objects.size());
  Objects[Id] = Obj;
}

void Module::addType(uint32_t Id, Type *T)
{
  assert(!Types.count(Id));
  Types[Id] = T;
}

void Module::addVariable(uint32_t Id, const Type *Ty, uint32_t Initializer)
{
  switch (Ty->getStorageClass())
  {
  case SpvStorageClassStorageBuffer:
  {
    BufferVariable V;

    // Variable may already have been created by decorations
    if (BufferVariables.count(Id))
      V = BufferVariables[Id];

    V.Ty = Ty;

    BufferVariables[Id] = V;
    break;
  }
  case SpvStorageClassInput:
  {
    InputVariable V;

    // Variable may already have been created by decorations
    if (InputVariables.count(Id))
      V = InputVariables[Id];

    V.Ty = Ty;

    InputVariables[Id] = V;
    break;
  }
  case SpvStorageClassPrivate:
  {
    assert(!PrivateVariables.count(Id));
    PrivateVariable V;
    V.Ty = Ty;
    V.Initializer = Initializer;
    PrivateVariables[Id] = V;
    break;
  }
  default:
    std::cout << "Unhandled storage class " << Ty->getStorageClass()
              << std::endl;
  }
}

std::vector<Object> Module::cloneObjects() const
{
  std::vector<Object> ClonedObjects;
  ClonedObjects.resize(Objects.size());
  for (size_t i = 0; i < Objects.size(); i++)
  {
    if (Objects[i].isSet())
      ClonedObjects[i] = Objects[i].clone();
  }
  return ClonedObjects;
}

const Function *Module::getEntryPoint(const std::string &Name) const
{
  if (!EntryPoints.count(Name))
    return nullptr;
  return Functions.at(EntryPoints.at(Name)).get();
}

const Function *Module::getFunction(uint32_t Id) const
{
  if (!Functions.count(Id))
    return nullptr;
  return Functions.at(Id).get();
}

const BufferVariableMap &Module::getBufferVariables() const
{
  return BufferVariables;
}

const InputVariableMap &Module::getInputVariables() const
{
  return InputVariables;
}

const PrivateVariableMap &Module::getPrivateVariables() const
{
  return PrivateVariables;
}

const Object &Module::getObject(uint32_t Id) const { return Objects.at(Id); }

const Type *Module::getType(uint32_t Id) const
{
  assert(Types.count(Id));
  return Types.at(Id);
}

std::unique_ptr<Module> Module::load(const uint32_t *Words, uint32_t NumWords)
{
  spvtools::Context SPVContext(SPV_ENV_UNIVERSAL_1_2);
  spv_diagnostic Diagnostic = nullptr;

  // Validate binary.
  if (spvValidateBinary(SPVContext.CContext(), Words, NumWords, &Diagnostic))
  {
    spvDiagnosticPrint(Diagnostic);
    return nullptr;
  }

  // Parse binary.
  ModuleBuilder MB;
  spvBinaryParse(SPVContext.CContext(), &MB, Words, NumWords, HandleHeader,
                 HandleInstruction, &Diagnostic);
  if (Diagnostic)
  {
    spvDiagnosticPrint(Diagnostic);
    return nullptr;
  }

  return MB.takeModule();
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

  // Check for SPIR-V magic number.
  if (Words[0] == 0x07230203)
    return load(Words, NumBytes / 4);

  // Assume file is in textual SPIR-V format.
  // Assemble it to a SPIR-V binary in memory.
  spv_binary Binary;
  spv_diagnostic Diagnostic = nullptr;
  spvtools::Context SPVContext(SPV_ENV_UNIVERSAL_1_2);
  spvTextToBinary(SPVContext.CContext(), (const char *)Words, NumBytes, &Binary,
                  &Diagnostic);
  if (Diagnostic)
  {
    spvDiagnosticPrint(Diagnostic);
    spvBinaryDestroy(Binary);
    return nullptr;
  }

  // Load and return Module.
  std::unique_ptr<Module> M = load(Binary->code, Binary->wordCount);
  spvBinaryDestroy(Binary);
  return M;
}

void Module::setBinding(uint32_t Variable, uint32_t Binding)
{
  BufferVariables[Variable].Binding = Binding;
}

void Module::setBuiltin(uint32_t Id, uint32_t Builtin)
{
  switch (Builtin)
  {
  case SpvBuiltInGlobalInvocationId:
    assert(!InputVariables.count(Id));
    InputVariables[Id].Builtin = Builtin;
    break;
  case SpvBuiltInWorkgroupSize:
    // TODO: Handle this?
    break;
  default:
    std::cout << "Unhandled builtin decoration: " << Builtin << std::endl;
  }
}

void Module::setDescriptorSet(uint32_t Variable, uint32_t DescriptorSet)
{
  BufferVariables[Variable].DescriptorSet = DescriptorSet;
}

} // namespace talvos
