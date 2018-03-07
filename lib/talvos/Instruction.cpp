// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file Instruction.cpp
/// This file defines the Instruction class.

#include <iostream>
#include <spirv/unified1/spirv.h>

#include "talvos/Instruction.h"

namespace talvos
{

Instruction::Instruction(uint16_t Opcode, uint16_t NumOperands,
                         const uint32_t *Operands, const Type *ResultType)
{
  this->Opcode = Opcode;
  this->NumOperands = NumOperands;
  this->ResultType = ResultType;
  this->Next = nullptr;

  this->Operands = new uint32_t[NumOperands];
  memcpy(this->Operands, Operands, NumOperands * sizeof(uint32_t));
}

void Instruction::insertAfter(Instruction *I)
{
  this->Next = std::move(I->Next);
  I->Next = std::unique_ptr<Instruction>(this);
}

void Instruction::print(std::ostream &O) const
{
  // TODO: Adapt whitespace here based on module ID bound
  if (ResultType)
    O << "  %" << Operands[1] << " = ";
  else
    O << "        ";

  O << opcodeToString(Opcode);
  for (unsigned i = 0; i < NumOperands; i++)
  {
    if (ResultType && i == 1)
      continue;
    O << " %" << Operands[i];
  }
}

const char *Instruction::opcodeToString(uint16_t Opcode)
{
  // TODO: SPIRV-Tools might expose spvOpcodeString at some point.
  switch (Opcode)
  {
#define CASE(OP)                                                               \
  case SpvOp##OP:                                                              \
    return "Op" #OP

    CASE(Nop);
    CASE(Undef);
    CASE(SourceContinued);
    CASE(Source);
    CASE(SourceExtension);
    CASE(Name);
    CASE(MemberName);
    CASE(String);
    CASE(Line);
    CASE(Extension);
    CASE(ExtInstImport);
    CASE(ExtInst);
    CASE(MemoryModel);
    CASE(EntryPoint);
    CASE(ExecutionMode);
    CASE(Capability);
    CASE(TypeVoid);
    CASE(TypeBool);
    CASE(TypeInt);
    CASE(TypeFloat);
    CASE(TypeVector);
    CASE(TypeMatrix);
    CASE(TypeImage);
    CASE(TypeSampler);
    CASE(TypeSampledImage);
    CASE(TypeArray);
    CASE(TypeRuntimeArray);
    CASE(TypeStruct);
    CASE(TypeOpaque);
    CASE(TypePointer);
    CASE(TypeFunction);
    CASE(TypeEvent);
    CASE(TypeDeviceEvent);
    CASE(TypeReserveId);
    CASE(TypeQueue);
    CASE(TypePipe);
    CASE(TypeForwardPointer);
    CASE(ConstantTrue);
    CASE(ConstantFalse);
    CASE(Constant);
    CASE(ConstantComposite);
    CASE(ConstantSampler);
    CASE(ConstantNull);
    CASE(SpecConstantTrue);
    CASE(SpecConstantFalse);
    CASE(SpecConstant);
    CASE(SpecConstantComposite);
    CASE(SpecConstantOp);
    CASE(Function);
    CASE(FunctionParameter);
    CASE(FunctionEnd);
    CASE(FunctionCall);
    CASE(Variable);
    CASE(ImageTexelPointer);
    CASE(Load);
    CASE(Store);
    CASE(CopyMemory);
    CASE(CopyMemorySized);
    CASE(AccessChain);
    CASE(InBoundsAccessChain);
    CASE(PtrAccessChain);
    CASE(ArrayLength);
    CASE(GenericPtrMemSemantics);
    CASE(InBoundsPtrAccessChain);
    CASE(Decorate);
    CASE(MemberDecorate);
    CASE(DecorationGroup);
    CASE(GroupDecorate);
    CASE(GroupMemberDecorate);
    CASE(VectorExtractDynamic);
    CASE(VectorInsertDynamic);
    CASE(VectorShuffle);
    CASE(CompositeConstruct);
    CASE(CompositeExtract);
    CASE(CompositeInsert);
    CASE(CopyObject);
    CASE(Transpose);
    CASE(SampledImage);
    CASE(ImageSampleImplicitLod);
    CASE(ImageSampleExplicitLod);
    CASE(ImageSampleDrefImplicitLod);
    CASE(ImageSampleDrefExplicitLod);
    CASE(ImageSampleProjImplicitLod);
    CASE(ImageSampleProjExplicitLod);
    CASE(ImageSampleProjDrefImplicitLod);
    CASE(ImageSampleProjDrefExplicitLod);
    CASE(ImageFetch);
    CASE(ImageGather);
    CASE(ImageDrefGather);
    CASE(ImageRead);
    CASE(ImageWrite);
    CASE(Image);
    CASE(ImageQueryFormat);
    CASE(ImageQueryOrder);
    CASE(ImageQuerySizeLod);
    CASE(ImageQuerySize);
    CASE(ImageQueryLod);
    CASE(ImageQueryLevels);
    CASE(ImageQuerySamples);
    CASE(ConvertFToU);
    CASE(ConvertFToS);
    CASE(ConvertSToF);
    CASE(ConvertUToF);
    CASE(UConvert);
    CASE(SConvert);
    CASE(FConvert);
    CASE(QuantizeToF16);
    CASE(ConvertPtrToU);
    CASE(SatConvertSToU);
    CASE(SatConvertUToS);
    CASE(ConvertUToPtr);
    CASE(PtrCastToGeneric);
    CASE(GenericCastToPtr);
    CASE(GenericCastToPtrExplicit);
    CASE(Bitcast);
    CASE(SNegate);
    CASE(FNegate);
    CASE(IAdd);
    CASE(FAdd);
    CASE(ISub);
    CASE(FSub);
    CASE(IMul);
    CASE(FMul);
    CASE(UDiv);
    CASE(SDiv);
    CASE(FDiv);
    CASE(UMod);
    CASE(SRem);
    CASE(SMod);
    CASE(FRem);
    CASE(FMod);
    CASE(VectorTimesScalar);
    CASE(MatrixTimesScalar);
    CASE(VectorTimesMatrix);
    CASE(MatrixTimesVector);
    CASE(MatrixTimesMatrix);
    CASE(OuterProduct);
    CASE(Dot);
    CASE(IAddCarry);
    CASE(ISubBorrow);
    CASE(UMulExtended);
    CASE(SMulExtended);
    CASE(Any);
    CASE(All);
    CASE(IsNan);
    CASE(IsInf);
    CASE(IsFinite);
    CASE(IsNormal);
    CASE(SignBitSet);
    CASE(LessOrGreater);
    CASE(Ordered);
    CASE(Unordered);
    CASE(LogicalEqual);
    CASE(LogicalNotEqual);
    CASE(LogicalOr);
    CASE(LogicalAnd);
    CASE(LogicalNot);
    CASE(Select);
    CASE(IEqual);
    CASE(INotEqual);
    CASE(UGreaterThan);
    CASE(SGreaterThan);
    CASE(UGreaterThanEqual);
    CASE(SGreaterThanEqual);
    CASE(ULessThan);
    CASE(SLessThan);
    CASE(ULessThanEqual);
    CASE(SLessThanEqual);
    CASE(FOrdEqual);
    CASE(FUnordEqual);
    CASE(FOrdNotEqual);
    CASE(FUnordNotEqual);
    CASE(FOrdLessThan);
    CASE(FUnordLessThan);
    CASE(FOrdGreaterThan);
    CASE(FUnordGreaterThan);
    CASE(FOrdLessThanEqual);
    CASE(FUnordLessThanEqual);
    CASE(FOrdGreaterThanEqual);
    CASE(FUnordGreaterThanEqual);
    CASE(ShiftRightLogical);
    CASE(ShiftRightArithmetic);
    CASE(ShiftLeftLogical);
    CASE(BitwiseOr);
    CASE(BitwiseXor);
    CASE(BitwiseAnd);
    CASE(Not);
    CASE(BitFieldInsert);
    CASE(BitFieldSExtract);
    CASE(BitFieldUExtract);
    CASE(BitReverse);
    CASE(BitCount);
    CASE(DPdx);
    CASE(DPdy);
    CASE(Fwidth);
    CASE(DPdxFine);
    CASE(DPdyFine);
    CASE(FwidthFine);
    CASE(DPdxCoarse);
    CASE(DPdyCoarse);
    CASE(FwidthCoarse);
    CASE(EmitVertex);
    CASE(EndPrimitive);
    CASE(EmitStreamVertex);
    CASE(EndStreamPrimitive);
    CASE(ControlBarrier);
    CASE(MemoryBarrier);
    CASE(AtomicLoad);
    CASE(AtomicStore);
    CASE(AtomicExchange);
    CASE(AtomicCompareExchange);
    CASE(AtomicCompareExchangeWeak);
    CASE(AtomicIIncrement);
    CASE(AtomicIDecrement);
    CASE(AtomicIAdd);
    CASE(AtomicISub);
    CASE(AtomicSMin);
    CASE(AtomicUMin);
    CASE(AtomicSMax);
    CASE(AtomicUMax);
    CASE(AtomicAnd);
    CASE(AtomicOr);
    CASE(AtomicXor);
    CASE(Phi);
    CASE(LoopMerge);
    CASE(SelectionMerge);
    CASE(Label);
    CASE(Branch);
    CASE(BranchConditional);
    CASE(Switch);
    CASE(Kill);
    CASE(Return);
    CASE(ReturnValue);
    CASE(Unreachable);
    CASE(LifetimeStart);
    CASE(LifetimeStop);
    CASE(GroupAsyncCopy);
    CASE(GroupWaitEvents);
    CASE(GroupAll);
    CASE(GroupAny);
    CASE(GroupBroadcast);
    CASE(GroupIAdd);
    CASE(GroupFAdd);
    CASE(GroupFMin);
    CASE(GroupUMin);
    CASE(GroupSMin);
    CASE(GroupFMax);
    CASE(GroupUMax);
    CASE(GroupSMax);
    CASE(ReadPipe);
    CASE(WritePipe);
    CASE(ReservedReadPipe);
    CASE(ReservedWritePipe);
    CASE(ReserveReadPipePackets);
    CASE(ReserveWritePipePackets);
    CASE(CommitReadPipe);
    CASE(CommitWritePipe);
    CASE(IsValidReserveId);
    CASE(GetNumPipePackets);
    CASE(GetMaxPipePackets);
    CASE(GroupReserveReadPipePackets);
    CASE(GroupReserveWritePipePackets);
    CASE(GroupCommitReadPipe);
    CASE(GroupCommitWritePipe);
    CASE(EnqueueMarker);
    CASE(EnqueueKernel);
    CASE(GetKernelNDrangeSubGroupCount);
    CASE(GetKernelNDrangeMaxSubGroupSize);
    CASE(GetKernelWorkGroupSize);
    CASE(GetKernelPreferredWorkGroupSizeMultiple);
    CASE(RetainEvent);
    CASE(ReleaseEvent);
    CASE(CreateUserEvent);
    CASE(IsValidEvent);
    CASE(SetUserEventStatus);
    CASE(CaptureEventProfilingInfo);
    CASE(GetDefaultQueue);
    CASE(BuildNDRange);
    CASE(ImageSparseSampleImplicitLod);
    CASE(ImageSparseSampleExplicitLod);
    CASE(ImageSparseSampleDrefImplicitLod);
    CASE(ImageSparseSampleDrefExplicitLod);
    CASE(ImageSparseSampleProjImplicitLod);
    CASE(ImageSparseSampleProjExplicitLod);
    CASE(ImageSparseSampleProjDrefImplicitLod);
    CASE(ImageSparseSampleProjDrefExplicitLod);
    CASE(ImageSparseFetch);
    CASE(ImageSparseGather);
    CASE(ImageSparseDrefGather);
    CASE(ImageSparseTexelsResident);
    CASE(NoLine);
    CASE(AtomicFlagTestAndSet);
    CASE(AtomicFlagClear);
    CASE(ImageSparseRead);
    CASE(SizeOf);
    CASE(TypePipeStorage);
    CASE(ConstantPipeStorage);
    CASE(CreatePipeFromPipeStorage);
    CASE(GetKernelLocalSizeForSubgroupCount);
    CASE(GetKernelMaxNumSubgroups);
    CASE(TypeNamedBarrier);
    CASE(NamedBarrierInitialize);
    CASE(MemoryNamedBarrier);
    CASE(ModuleProcessed);
    CASE(ExecutionModeId);
    CASE(DecorateId);
    CASE(SubgroupBallotKHR);
    CASE(SubgroupFirstInvocationKHR);
    CASE(SubgroupAllKHR);
    CASE(SubgroupAnyKHR);
    CASE(SubgroupAllEqualKHR);
    CASE(SubgroupReadInvocationKHR);
    CASE(GroupIAddNonUniformAMD);
    CASE(GroupFAddNonUniformAMD);
    CASE(GroupFMinNonUniformAMD);
    CASE(GroupUMinNonUniformAMD);
    CASE(GroupSMinNonUniformAMD);
    CASE(GroupFMaxNonUniformAMD);
    CASE(GroupUMaxNonUniformAMD);
    CASE(GroupSMaxNonUniformAMD);
    CASE(FragmentMaskFetchAMD);
    CASE(FragmentFetchAMD);
    CASE(SubgroupShuffleINTEL);
    CASE(SubgroupShuffleDownINTEL);
    CASE(SubgroupShuffleUpINTEL);
    CASE(SubgroupShuffleXorINTEL);
    CASE(SubgroupBlockReadINTEL);
    CASE(SubgroupBlockWriteINTEL);
    CASE(SubgroupImageBlockReadINTEL);
    CASE(SubgroupImageBlockWriteINTEL);
  default:
    return "<invalid>";

#undef CASE
  }
}

} // namespace talvos
