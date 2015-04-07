//===------------------- include/Reader/abi.cpp -----------------*- C++ -*-===//
//
// LLILC
//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Defines the ABI abstraction used when lowering functions to LLVM IR.
///
//===----------------------------------------------------------------------===//

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "reader.h"
#include "readerir.h"
#include "abi.h"
#include <cstdint>
#include <cassert>

using namespace llvm;

// Static class with helpers for the Microsoft x86-64 ABI.
class X86_64_Win64 {
private:
  X86_64_Win64() {}
  static ABIArgInfo classify(Type *Ty, const DataLayout &DL);

public:
  static void computeSignatureInfo(Type *ResultType, ArrayRef<Type *> ArgTypes,
                                   const DataLayout &DL, ABIArgInfo &ResultInfo,
                                   std::vector<ABIArgInfo> &ArgInfos);
};

// Static class wqith helpers for the System V x86-64 ABI.
class X86_64_SysV {
private:
  X86_64_SysV() {}

public:
  static void computeSignatureInfo(Type *ResultType, ArrayRef<Type *> ArgTypes,
                                   const DataLayout &DL, ABIArgInfo &ResultInfo,
                                   std::vector<ABIArgInfo> &ArgInfos);
};

class X86_64ABIInfo : public ABIInfo {
private:
  bool IsWindows;
  const DataLayout &TheDataLayout;

public:
  X86_64ABIInfo(Triple TargetTriple, const DataLayout &DL);

  void computeSignatureInfo(CallingConv::ID CC, Type *ResultType,
                            ArrayRef<Type *> ArgTypes, ABIArgInfo &ResultInfo,
                            std::vector<ABIArgInfo> &ArgInfos) const override;
};

ABIArgInfo X86_64_Win64::classify(Type *Ty, const DataLayout &DL) {
  if (Ty->isAggregateType()) {
    // If the aggregate's size in bytes is a power of 2 that is less than or
    // equal to 8, it can be passed directly once coerced to an
    // appropriately-sized
    // integer. Otherwise, it must be passed indirectly.
    uint64_t SizeInBits = DL.getTypeSizeInBits(Ty);
    uint64_t SizeInBytes = SizeInBits / 8;
    if (SizeInBytes <= 8 && llvm::isPowerOf2_64(SizeInBytes)) {
      return ABIArgInfo::getDirect(
          IntegerType::get(Ty->getContext(), SizeInBits));
    }

    return ABIArgInfo::getIndirect(Ty);
  }

  if (Ty->isIntegerTy()) {
    assert(Ty->getIntegerBitWidth() <= 64);
    return ABIArgInfo::getDirect(Ty);
  }

  if (Ty->isFloatingPointTy()) {
    assert(Ty->isFloatTy() || Ty->isDoubleTy());
    return ABIArgInfo::getDirect(Ty);
  }

  // TODO: vector types
  assert(Ty->isPointerTy() || Ty->isVoidTy());
  return ABIArgInfo::getDirect(Ty);
}

void X86_64_Win64::computeSignatureInfo(Type *ResultType,
                                        ArrayRef<Type *> ArgTypes,
                                        const DataLayout &DL,
                                        ABIArgInfo &ResultInfo,
                                        std::vector<ABIArgInfo> &ArgInfos) {
  ResultInfo = classify(ResultType, DL);

  for (auto &Arg : ArgTypes) {
    ArgInfos.push_back(classify(Arg, DL));
  }
}

void X86_64_SysV::computeSignatureInfo(Type *ResultType,
                                       ArrayRef<Type *> ArgTypes,
                                       const DataLayout &DL,
                                       ABIArgInfo &ResultInfo,
                                       std::vector<ABIArgInfo> &ArgInfos) {
  // TODO: RyuJIT does not implement the SysV ABI rules as decribed in "System V
  //       Application Binary Interface". For now, agree and just use the Win64
  //       rules.
  X86_64_Win64::computeSignatureInfo(ResultType, ArgTypes, DL, ResultInfo,
                                     ArgInfos);
}

X86_64ABIInfo::X86_64ABIInfo(Triple TargetTriple, const DataLayout &DL)
    : TheDataLayout(DL) {
  assert(TargetTriple.getArch() == Triple::x86_64);
  IsWindows = TargetTriple.isOSWindows();
}

void X86_64ABIInfo::computeSignatureInfo(
    CallingConv::ID CC, Type *ResultType, ArrayRef<Type *> ArgTypes,
    ABIArgInfo &ResultInfo, std::vector<ABIArgInfo> &ArgInfos) const {
  if (CC == CallingConv::C) {
    CC = IsWindows ? CallingConv::X86_64_Win64 : CallingConv::X86_64_SysV;
  }

  switch (CC) {
  case CallingConv::X86_64_Win64:
    X86_64_Win64::computeSignatureInfo(ResultType, ArgTypes, TheDataLayout,
                                       ResultInfo, ArgInfos);
    break;

  case CallingConv::X86_64_SysV:
    X86_64_SysV::computeSignatureInfo(ResultType, ArgTypes, TheDataLayout,
                                      ResultInfo, ArgInfos);
    break;

  default:
    assert(CC != CallingConv::C);
    assert(false && "Unsupported calling convention");
  }
}

ABIInfo *ABIInfo::get(Module &M) {
  Triple TargetTriple(M.getTargetTriple());

  switch (TargetTriple.getArch()) {
  case Triple::x86_64:
    return new X86_64ABIInfo(TargetTriple, M.getDataLayout());

  default:
    assert(false && "Unsupported architecture");
  }
}

ABIArgInfo::ABIArgInfo(Kind TheKind, Type *TheType)
    : TheKind(TheKind), TheType(TheType) {}

ABIArgInfo ABIArgInfo::getDirect(llvm::Type *TheType) {
  return ABIArgInfo(Kind::Direct, TheType);
}

ABIArgInfo ABIArgInfo::getIndirect(llvm::Type *TheType) {
  return ABIArgInfo(Kind::Indirect, TheType);
}

ABIArgInfo::Kind ABIArgInfo::getKind() const { return TheKind; }

Type *ABIArgInfo::getType() const { return TheType; }

void ABIArgInfo::setIndex(uint32_t Index) { this->Index = Index; }

uint32_t ABIArgInfo::getIndex() const { return Index; }
