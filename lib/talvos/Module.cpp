// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Module.cpp
/// This file defines the Module class.

#include <cassert>
#include <cstdio>
#include <iostream>
#include <spirv-tools/libspirv.hpp>

#include <spirv/unified1/spirv.h>

#include "talvos/Block.h"
#include "talvos/Function.h"
#include "talvos/Instruction.h"
#include "talvos/Module.h"
#include "talvos/Type.h"

namespace talvos
{

/// Internal class used to construct a Module during SPIRV-Tools parsing.
class ModuleBuilder
{
public:
  /// Initialize the module builder.
  void init(uint32_t IdBound)
  {
    assert(!Mod && "Module already initialized");
    Mod = std::unique_ptr<Module>(new Module(IdBound));
    CurrentFunction = nullptr;
    CurrentBlock = nullptr;
    PreviousInstruction = nullptr;
  }

  /// Process a parsed SPIR-V instruction.
  void processInstruction(const spv_parsed_instruction_t *Inst)
  {
    assert(Mod);

    if (Inst->opcode == SpvOpFunction)
    {
      assert(CurrentFunction == nullptr);
      const Type *FuncType =
          Mod->getType(Inst->words[Inst->operands[3].offset]);
      CurrentFunction = std::make_unique<Function>(Inst->result_id, FuncType);
    }
    else if (Inst->opcode == SpvOpFunctionEnd)
    {
      assert(CurrentFunction);
      assert(CurrentBlock);
      CurrentFunction->addBlock(std::move(CurrentBlock));
      Mod->addFunction(std::move(CurrentFunction));
      CurrentFunction = nullptr;
      CurrentBlock = nullptr;
    }
    else if (Inst->opcode == SpvOpFunctionParameter)
    {
      CurrentFunction->addParam(Inst->result_id);
    }
    else if (Inst->opcode == SpvOpLabel)
    {
      if (CurrentBlock)
        // Add previous block to function.
        CurrentFunction->addBlock(std::move(CurrentBlock));
      else
        // First block - set as entry block.
        CurrentFunction->setFirstBlock(Inst->result_id);

      // Create new block.
      CurrentBlock = std::make_unique<Block>(Inst->result_id);
      PreviousInstruction = &CurrentBlock->getLabel();
    }
    else if (CurrentFunction)
    {
      // Create an array of operand values.
      uint32_t *Operands = new uint32_t[Inst->num_operands];
      for (int i = 0; i < Inst->num_operands; i++)
      {
        // TODO: Handle larger operands
        assert(Inst->operands[i].num_words == 1);
        Operands[i] = Inst->words[Inst->operands[i].offset];
      }

      // Create the instruction.
      const Type *ResultType =
          Inst->type_id ? Mod->getType(Inst->type_id) : nullptr;
      Instruction *I = new Instruction(Inst->opcode, Inst->num_operands,
                                       Operands, ResultType);
      delete[] Operands;

      // Insert this instruction into the current block.
      assert(PreviousInstruction);
      I->insertAfter(PreviousInstruction);
      PreviousInstruction = I;
    }
    else
    {
      switch (Inst->opcode)
      {
      case SpvOpCapability:
      {
        uint32_t Capability = Inst->words[Inst->operands[0].offset];
        switch (Capability)
        {
        case SpvCapabilityInt16:
        case SpvCapabilityInt64:
        case SpvCapabilityFloat64:
        case SpvCapabilityShader:
        case SpvCapabilityVariablePointers:
        case SpvCapabilityVariablePointersStorageBuffer:
          break;
        default:
          std::cerr << "WARNING: Unrecognized capability " << Capability
                    << std::endl;
        }
        break;
      }
      case SpvOpConstant:
      case SpvOpSpecConstant: // TODO: Handle specialization constants
      {
        const Type *Ty = Mod->getType(Inst->type_id);

        Object Constant = Object(Ty);
        uint16_t Offset = Inst->operands[2].offset;
        switch (Ty->getSize())
        {
        case 4:
          Constant.set<uint32_t>(Inst->words[Offset]);
          break;
        case 8:
          Constant.set<uint64_t>(*(uint64_t *)(Inst->words + Offset));
          break;
        default:
          std::cerr << "Unhandled OpConstant type size:" << Ty->getSize()
                    << std::endl;
          abort();
        }
        Mod->addObject(Inst->result_id, Constant);
        break;
      }
      case SpvOpConstantComposite:
      case SpvOpSpecConstantComposite: // TODO: Handle specialization constants
      {
        const Type *Ty = Mod->getType(Inst->type_id);

        // Create composite object.
        Object Composite(Ty);

        // Set constituent values.
        for (uint32_t i = 2; i < Inst->num_operands; i++)
        {
          uint32_t Id = Inst->words[Inst->operands[i].offset];
          Composite.insert({i - 2}, Mod->getObject(Id));
        }

        // Add object to module.
        Mod->addObject(Inst->result_id, Composite);
        break;
      }
      case SpvOpConstantFalse:
      {
        const Type *Ty = Mod->getType(Inst->type_id);
        Mod->addObject(Inst->result_id, Object(Ty, false));
        break;
      }
      case SpvOpConstantNull:
      {
        // Create and add object.
        const Type *Ty = Mod->getType(Inst->type_id);
        Object Value(Ty);
        Value.zero();
        Mod->addObject(Inst->result_id, Value);
        break;
      }
      case SpvOpConstantTrue:
      {
        const Type *Ty = Mod->getType(Inst->type_id);
        Mod->addObject(Inst->result_id, Object(Ty, true));
        break;
      }
      case SpvOpDecorate:
      {
        uint32_t Target = Inst->words[Inst->operands[0].offset];
        uint32_t Decoration = Inst->words[Inst->operands[1].offset];
        switch (Decoration)
        {
        case SpvDecorationArrayStride:
          ArrayStrides[Target] = Inst->words[Inst->operands[2].offset];
          break;
        case SpvDecorationBinding:
          Mod->setBinding(Target, Inst->words[Inst->operands[2].offset]);
          break;
        case SpvDecorationBlock:
        case SpvDecorationBufferBlock:
          // TODO: Need to handle these?
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
      case SpvOpExecutionMode:
      {
        uint32_t Entry = Inst->words[Inst->operands[0].offset];
        uint32_t Mode = Inst->words[Inst->operands[1].offset];
        switch (Mode)
        {
        case SpvExecutionModeLocalSize:
          Mod->addLocalSize(Entry,
                            Dim3(Inst->words + Inst->operands[2].offset));
          break;
        default:
          std::cerr << "Unimplemented execution mode: " << Mode << std::endl;
          abort();
        }
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
      case SpvOpExtInstImport:
      {
        // TODO: Store the mapping from result ID to set for later use
        char *ExtInstSet = (char *)(Inst->words + Inst->operands[1].offset);
        if (strcmp(ExtInstSet, "GLSL.std.450"))
        {
          std::cerr << "WARNING: Unrecognized extended instruction set "
                    << ExtInstSet << std::endl;
        }
        break;
      }
      case SpvOpLine:
        // TODO: Do something with this
        break;
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
      case SpvOpMemberName:
        // TODO: Do something with this
        break;
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
      case SpvOpModuleProcessed:
        break;
      case SpvOpName:
        // TODO: Do something with this
        break;
      case SpvOpNoLine:
        // TODO: Do something with this
        break;
      case SpvOpSource:
      case SpvOpSourceContinued:
        // TODO: Do something with these
        break;
      case SpvOpSourceExtension:
      {
        char *Extension = (char *)(Inst->words + Inst->operands[0].offset);
        if (strcmp(Extension, "GL_ARB_separate_shader_objects"))
        {
          std::cerr << "WARNING: Unrecognized extension " << Extension
                    << std::endl;
        }
        break;
      }
      case SpvOpString:
        // TODO: Do something with this
        break;
      case SpvOpTypeArray:
      {
        const Type *ElemType =
            Mod->getType(Inst->words[Inst->operands[1].offset]);
        uint32_t LengthId = Inst->words[Inst->operands[2].offset];
        uint32_t Length = Mod->getObject(LengthId).get<uint32_t>();
        uint32_t ArrayStride = (uint32_t)ElemType->getSize();
        if (ArrayStrides.count(Inst->result_id))
          ArrayStride = ArrayStrides[Inst->result_id];
        Mod->addType(Inst->result_id,
                     Type::getArray(ElemType, Length, ArrayStride));
        break;
      }
      case SpvOpTypeBool:
      {
        Mod->addType(Inst->result_id, Type::getBool());
        break;
      }
      case SpvOpTypeFloat:
      {
        uint32_t Width = Inst->words[Inst->operands[1].offset];
        Mod->addType(Inst->result_id, Type::getFloat(Width));
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
        for (int i = 2; i < Inst->num_operands; i++)
        {
          uint32_t ArgType = Inst->words[Inst->operands[i].offset];
          ArgTypes.push_back(Mod->getType(ArgType));
        }
        Mod->addType(Inst->result_id,
                     Type::getFunction(Mod->getType(ReturnType), ArgTypes));
        break;
      }
      case SpvOpTypeMatrix:
      {
        const Type *ColumnType =
            Mod->getType(Inst->words[Inst->operands[1].offset]);
        uint32_t NumColumns = Inst->words[Inst->operands[2].offset];
        Mod->addType(Inst->result_id, Type::getMatrix(ColumnType, NumColumns));
        break;
      }
      case SpvOpTypePointer:
      {
        uint32_t StorageClass = Inst->words[Inst->operands[1].offset];
        const Type *ElemType =
            Mod->getType(Inst->words[Inst->operands[2].offset]);
        uint32_t ArrayStride = (uint32_t)ElemType->getSize();
        if (ArrayStrides.count(Inst->result_id))
          ArrayStride = ArrayStrides[Inst->result_id];
        Mod->addType(Inst->result_id,
                     Type::getPointer(StorageClass, ElemType, ArrayStride));
        break;
      }
      case SpvOpTypeRuntimeArray:
      {
        const Type *ElemType =
            Mod->getType(Inst->words[Inst->operands[1].offset]);
        uint32_t ArrayStride = (uint32_t)ElemType->getSize();
        if (ArrayStrides.count(Inst->result_id))
          ArrayStride = ArrayStrides[Inst->result_id];
        Mod->addType(Inst->result_id,
                     Type::getRuntimeArray(ElemType, ArrayStride));
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
      case SpvOpUndef:
        Mod->addObject(Inst->result_id, Object(Mod->getType(Inst->type_id)));
        break;
      case SpvOpVariable:
        Mod->addVariable(Inst->result_id, Mod->getType(Inst->type_id),
                         ((Inst->num_operands > 3)
                              ? Inst->words[Inst->operands[3].offset]
                              : 0));
        break;
      default:
        std::cerr << "Unhandled instruction: "
                  << Instruction::opcodeToString(Inst->opcode) << " ("
                  << Inst->opcode << ")" << std::endl;
        abort();
      }
    }
  };

  /// Returns the Module that has been built.
  std::unique_ptr<Module> takeModule()
  {
    assert(Mod);
    return std::move(Mod);
  }

private:
  /// Internal ModuleBuilder variables.
  ///\{
  std::unique_ptr<Module> Mod;
  std::unique_ptr<Function> CurrentFunction;
  std::unique_ptr<Block> CurrentBlock;
  Instruction *PreviousInstruction;
  std::map<uint32_t, uint32_t> ArrayStrides;
  std::map<std::pair<uint32_t, uint32_t>, uint32_t> MemberOffsets;
  ///\}
};

/// Callback for SPIRV-Tools parsing a SPIR-V header.
spv_result_t HandleHeader(void *user_data, spv_endianness_t endian,
                          uint32_t /* magic */, uint32_t version,
                          uint32_t generator, uint32_t id_bound,
                          uint32_t schema)
{
  ((ModuleBuilder *)user_data)->init(id_bound);
  return SPV_SUCCESS;
}

/// Callback for SPIRV-Tools parsing a SPIR-V instruction.
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

Module::~Module() {}

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

void Module::addLocalSize(uint32_t Entry, Dim3 LocalSize)
{
  assert(LocalSizes.count(Entry) == 0);
  LocalSizes[Entry] = LocalSize;
}

void Module::addObject(uint32_t Id, const Object &Obj)
{
  assert(Id < Objects.size());
  Objects[Id] = Obj;
}

void Module::addType(uint32_t Id, std::unique_ptr<Type> Ty)
{
  assert(!Types.count(Id));
  Types[Id] = std::move(Ty);
}

void Module::addVariable(uint32_t Id, const Type *Ty, uint32_t Initializer)
{
  switch (Ty->getStorageClass())
  {
  case SpvStorageClassStorageBuffer:
  case SpvStorageClassUniform:
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

    // Variable must have already been created by a builtin decoration.
    assert(InputVariables.count(Id));
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
  case SpvStorageClassWorkgroup:
  {
    assert(!WorkgroupVariables.count(Id));
    WorkgroupVariables[Id] = Ty;
    break;
  }
  default:
    std::cout << "Unhandled storage class " << Ty->getStorageClass()
              << std::endl;
  }
}

const Function *Module::getEntryPoint(const std::string &Name) const
{
  if (!EntryPoints.count(Name))
    return nullptr;
  return Functions.at(EntryPoints.at(Name)).get();
}

std::string Module::getEntryPointName(uint32_t Id) const
{
  for (auto &E : EntryPoints)
  {
    if (E.second == Id)
      return E.first;
  }
  return "";
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

Dim3 Module::getLocalSize(uint32_t Entry) const
{
  if (LocalSizes.count(Entry))
    return LocalSizes.at(Entry);
  else
    return Dim3(1, 1, 1);
}

const Object &Module::getObject(uint32_t Id) const { return Objects.at(Id); }

const std::vector<Object> &Module::getObjects() const { return Objects; }

const Type *Module::getType(uint32_t Id) const
{
  if (Types.count(Id) == 0)
    return nullptr;
  return Types.at(Id).get();
}

const WorkgroupVariableMap &Module::getWorkgroupVariables() const
{
  return WorkgroupVariables;
}

std::unique_ptr<Module> Module::load(const uint32_t *Words, size_t NumWords)
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
  std::vector<uint8_t> Bytes;
  fseek(SPVFile, 0, SEEK_END);
  long NumBytes = ftell(SPVFile);
  Bytes.resize(NumBytes);
  fseek(SPVFile, 0, SEEK_SET);
  fread(Bytes.data(), 1, NumBytes, SPVFile);
  fclose(SPVFile);

  // Check for SPIR-V magic number.
  if (((uint32_t *)Bytes.data())[0] == 0x07230203)
    return load((uint32_t *)Bytes.data(), NumBytes / 4);

  // Assume file is in textual SPIR-V format.
  // Assemble it to a SPIR-V binary in memory.
  spv_binary Binary;
  spv_diagnostic Diagnostic = nullptr;
  spvtools::Context SPVContext(SPV_ENV_UNIVERSAL_1_2);
  spvTextToBinary(SPVContext.CContext(), (const char *)Bytes.data(), NumBytes,
                  &Binary, &Diagnostic);
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
  case SpvBuiltInLocalInvocationId:
  case SpvBuiltInNumWorkgroups:
  case SpvBuiltInWorkgroupId:
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
