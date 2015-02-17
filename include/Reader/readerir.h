//===------------------- include/Reader/readerir.h --------------*- C++ -*-===//
//
// LLVM-MSILC
//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.
//
//===----------------------------------------------------------------------===//
//
// Declares the GenIR class, which overrides ReaderBase to generate LLVM IR
// from MSIL bytecode.
//
//===----------------------------------------------------------------------===//

#ifndef MSIL_READER_IR_H
#define MSIL_READER_IR_H

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/CFG.h"
#include "reader.h"
#include "MSILCJit.h"

class FlowGraphNode : public llvm::BasicBlock {};

struct FlowGraphNodeInfo {
public:
  uint32_t StartMSILOffset;
  uint32_t EndMSILOffset;
  bool IsVisited;
  ReaderStack *TheReaderStack;
};

class IRNode : public llvm::Value {};

class FlowGraphEdgeList {
public:
  FlowGraphEdgeList(){};

  virtual void moveNext() = 0;

  virtual FlowGraphNode *getSink() = 0;

  virtual FlowGraphNode *getSource() = 0;
};

class FlowGraphPredecessorEdgeList : public FlowGraphEdgeList {
public:
  FlowGraphPredecessorEdgeList(FlowGraphNode *Fg)
      : FlowGraphEdgeList(), PredIterator(Fg), PredIteratorEnd(Fg, true) {}

  void moveNext() override { PredIterator++; }

  FlowGraphNode *getSink() override {
    return (FlowGraphNode *)PredIterator.getUse().get();
  }

  FlowGraphNode *getSource() override {
    return (PredIterator == PredIteratorEnd) ? NULL
                                             : (FlowGraphNode *)*PredIterator;
  }

private:
  llvm::pred_iterator PredIterator;
  llvm::pred_iterator PredIteratorEnd;
};

class FlowGraphSuccessorEdgeList : public FlowGraphEdgeList {
public:
  FlowGraphSuccessorEdgeList(FlowGraphNode *Fg)
      : FlowGraphEdgeList(), SuccIterator(Fg->getTerminator()),
        SuccIteratorEnd(Fg->getTerminator(), true) {}

  void moveNext() override { SuccIterator++; }

  FlowGraphNode *getSink() override {
    return (SuccIterator == SuccIteratorEnd) ? NULL
                                             : (FlowGraphNode *)*SuccIterator;
  }

  FlowGraphNode *getSource() override {
    return (FlowGraphNode *)SuccIterator.getSource();
  }

private:
  llvm::succ_iterator SuccIterator;
  llvm::succ_iterator SuccIteratorEnd;
};

class GenStack : public ReaderStack {
public:
  GenStack();
  GenStack(uint32_t MaxStack, ReaderBase *Reader);
  IRNode *pop(void) override;
  void push(IRNode *NewVal, IRNode **NewIR) override;
  void clearStack(void) override;
  bool empty(void) override;
  void assertEmpty(void) override;
  uint32_t depth() override;

  // For iteration
  IRNode *getIterator(ReaderStackIterator **) override;
  IRNode *iteratorGetNext(ReaderStackIterator **) override;
  void iteratorReplace(ReaderStackIterator **, IRNode *) override;
  IRNode *getReverseIterator(ReaderStackIterator **) override;
  IRNode *getReverseIteratorFromDepth(ReaderStackIterator **,
                                      uint32_t Depth) override;
  IRNode *reverseIteratorGetNext(ReaderStackIterator **) override;

#if !defined(NODEBUG)
  void print() override;
#endif

  ReaderStack *copy() override;

private:
  int32_t Max;
  int32_t Top;
  IRNode **Stack;
  ReaderBase *Reader;
};

class GenIR : public ReaderBase {

public:
  GenIR(MSILCJitContext *JitContext,
        std::map<CORINFO_CLASS_HANDLE, llvm::Type *> *ClassTypeMap,
        std::map<std::tuple<CorInfoType, CORINFO_CLASS_HANDLE, uint32_t>,
                 llvm::Type *> *ArrayTypeMap,
        std::map<CORINFO_FIELD_HANDLE, uint32_t> *FieldIndexMap)
      : ReaderBase(JitContext->JitInfo, JitContext->MethodInfo,
                   JitContext->Flags) {
    this->JitContext = JitContext;
    this->ClassTypeMap = ClassTypeMap;
    this->ArrayTypeMap = ArrayTypeMap;
    this->FieldIndexMap = FieldIndexMap;
  }

  static bool isValidStackType(IRNode *Node);

  // ////////////////////////////////////////////////////////////////////
  //             IL generation methods supplied by the clients
  //
  // All pure virtual routines must be implemented by the client, non-pure
  // virtual routines have a default implementation in the reader, but can
  // be overloaded if necessary.
  // /////////////////////////////////////////////////////////////////////

  // MSIL Routines - client defined routines that are invoked by the reader.
  //                 One will be called for each msil opcode.

  uint32_t getPointerByteSize() override {
    return TargetPointerSizeInBits / 8;
  }

  void opcodeDebugPrint(uint8_t *Buf, unsigned StartOffset,
                        unsigned EndOffset) override {
    return;
  };

  // Used for testing, client can force verification.
  bool verForceVerification(void) override { return false; };

  bool abs(IRNode *Arg1, IRNode **RetVal, IRNode **NewIR) override {
    throw NotYetImplementedException("abs");
  };

  IRNode *argList(IRNode **NewIR) override {
    throw NotYetImplementedException("argList");
  };
  IRNode *instParam(IRNode **NewIR) override {
    throw NotYetImplementedException("instParam");
  };
  IRNode *secretParam(IRNode **NewIR) override {
    throw NotYetImplementedException("secretParam");
  };
  IRNode *thisObj(IRNode **NewIR) override {
    throw NotYetImplementedException("thisObj");
  };
  void boolBranch(ReaderBaseNS::BoolBranchOpcode Opcode, IRNode *Arg1,
                  IRNode **NewIR) override;

  IRNode *binaryOp(ReaderBaseNS::BinaryOpcode Opcode, IRNode *Arg1,
                   IRNode *Arg2, IRNode **NewIR) override;
  void branch(IRNode **NewIR) override;

  IRNode *call(ReaderBaseNS::CallOpcode Opcode, mdToken Token,
               mdToken ConstraintTypeRef, mdToken LoadFtnToken,
               bool HasReadOnlyPrefix, bool HasTailCallPrefix,
               bool IsUnmarkedTailCall, uint32_t CurrOffset,
               bool *RecursiveTailCall, IRNode **NewIR) override;

  IRNode *castOp(CORINFO_RESOLVED_TOKEN *ResolvedToken, IRNode *ObjRefNode,
                 IRNode **NewIR, CorInfoHelpFunc HelperId) override;

  IRNode *ckFinite(IRNode *Arg1, IRNode **NewIR) override {
    throw NotYetImplementedException("ckFinite");
  };
  IRNode *cmp(ReaderBaseNS::CmpOpcode Opode, IRNode *Arg1, IRNode *Arg2,
              IRNode **NewIR) override;
  void condBranch(ReaderBaseNS::CondBranchOpcode Opcode, IRNode *Arg1,
                  IRNode *Arg2, IRNode **NewIR) override;
  IRNode *conv(ReaderBaseNS::ConvOpcode Opcode, IRNode *Arg1,
               IRNode **NewIR) override;

  void dup(IRNode *Opr, IRNode **Result1, IRNode **Result2,
           IRNode **NewIR) override;
  void endFilter(IRNode *Arg1, IRNode **NewIR) override {
    throw NotYetImplementedException("endFilter");
  };

  FlowGraphNode *fgNodeGetNext(FlowGraphNode *FgNode) override;
  uint32_t fgNodeGetStartMSILOffset(FlowGraphNode *Fg) override;
  void fgNodeSetStartMSILOffset(FlowGraphNode *Fg,
                                uint32_t Offset) override;
  uint32_t fgNodeGetEndMSILOffset(FlowGraphNode *Fg) override;
  void fgNodeSetEndMSILOffset(FlowGraphNode *FgNode,
                              uint32_t Offset) override;

  bool fgNodeIsVisited(FlowGraphNode *FgNode) override;
  void fgNodeSetVisited(FlowGraphNode *FgNode, bool Visited) override;
  void fgNodeSetOperandStack(FlowGraphNode *Fg, ReaderStack *Stack) override;
  ReaderStack *fgNodeGetOperandStack(FlowGraphNode *Fg) override;

  IRNode *getStaticFieldAddress(CORINFO_RESOLVED_TOKEN *ResolvedToken,
                                IRNode **NewIR) override;

  void jmp(ReaderBaseNS::CallOpcode Opcode, mdToken Token, bool HasThis,
           bool HasVarArg, IRNode **NewIR) override {
    throw NotYetImplementedException("jmp");
  };

  void leave(uint32_t TargetOffset, bool IsNonLocal,
             bool EndsWithNonLocalGoto, IRNode **NewIR) override;
  IRNode *loadArg(uint32_t ArgOrdinal, bool IsJmp, IRNode **NewIR) override;
  IRNode *loadLocal(uint32_t ArgOrdinal, IRNode **NewIR) override;
  IRNode *loadArgAddress(uint32_t ArgOrdinal, IRNode **NewIR) override;
  IRNode *loadLocalAddress(uint32_t LocOrdinal, IRNode **NewIR) override;
  IRNode *loadConstantI4(int32_t Constant, IRNode **NewIR) override;
  IRNode *loadConstantI8(__int64 Constant, IRNode **NewIR) override;
  IRNode *loadConstantI(size_t Constant, IRNode **NewIR) override;
  IRNode *loadConstantR4(float Value, IRNode **NewIR) override;
  IRNode *loadConstantR8(double Value, IRNode **NewIR) override;
  IRNode *loadElem(ReaderBaseNS::LdElemOpcode Opcode,
                   CORINFO_RESOLVED_TOKEN *ResolvedToken, IRNode *Arg1,
                   IRNode *Arg2, IRNode **NewIR) override {
    throw NotYetImplementedException("loadElem");
  };
  IRNode *loadElemA(CORINFO_RESOLVED_TOKEN *ResolvedToken, IRNode *Arg1,
                    IRNode *Arg2, bool IsReadOnly, IRNode **NewIR) override {
    throw NotYetImplementedException("loadElemA");
  };
  IRNode *loadField(CORINFO_RESOLVED_TOKEN *ResolvedToken, IRNode *Arg1,
                    ReaderAlignType Alignment, bool IsVolatile,
                    IRNode **NewIR) override;

  IRNode *loadFuncptr(CORINFO_RESOLVED_TOKEN *ResolvedToken,
                      CORINFO_CALL_INFO *CallInfo, IRNode **NewIR) override {
    throw NotYetImplementedException("loadFuncptr");
  };
  IRNode *loadNull(IRNode **NewIR) override;
  IRNode *localAlloc(IRNode *Arg, bool ZeroInit, IRNode **NewIR) override {
    throw NotYetImplementedException("localAlloc");
  };
  IRNode *loadFieldAddress(CORINFO_RESOLVED_TOKEN *ResolvedToken, IRNode *Obj,
                           IRNode **NewIR) override;

  IRNode *loadLen(IRNode *Arg1, IRNode **NewIR) override;

  bool arrayAddress(CORINFO_SIG_INFO *Sig, IRNode **RetVal,
                    IRNode **NewIR) override {
    throw NotYetImplementedException("arrayAddress");
  };
  IRNode *loadStringLen(IRNode *Arg1, IRNode **NewIR) override;

  IRNode *getTypeFromHandle(IRNode *Arg1, IRNode **NewIR) override {
    throw NotYetImplementedException("getTypeFromHandle");
  };
  IRNode *getValueFromRuntimeHandle(IRNode *Arg1, IRNode **NewIR) override {
    throw NotYetImplementedException("getValueFromRuntimeHandle");
  };
  IRNode *arrayGetDimLength(IRNode *Arg1, IRNode *Arg2,
                            CORINFO_CALL_INFO *CallInfo,
                            IRNode **NewIR) override {
    throw NotYetImplementedException("arrayGetDimLength");
  };

  IRNode *loadAndBox(CORINFO_RESOLVED_TOKEN *ResolvedToken, IRNode *Addr,
                     ReaderAlignType AlignmentPrefix, IRNode **NewIR) override {
    throw NotYetImplementedException("loadAndBox");
  };
  IRNode *convertHandle(IRNode *GetTokenNumericNode, CorInfoHelpFunc HelperID,
                        CORINFO_CLASS_HANDLE ClassHandle,
                        IRNode **NewIR) override {
    throw NotYetImplementedException("convertHandle");
  };
  void convertTypeHandleLookupHelperToIntrinsic(bool CanCompareToGetType,
                                                IRNode *IR) override {
    throw NotYetImplementedException(
        "convertTypeHandleLookupHelperToIntrinsic");
  };

  IRNode *loadStaticField(CORINFO_RESOLVED_TOKEN *FieldToken, bool IsVolatile,
                          IRNode **NewIR) override;

  IRNode *stringLiteral(mdToken Token, void *StringHandle, InfoAccessType Iat,
                        IRNode **NewIR);

  IRNode *loadStr(mdToken Token, IRNode **NewIR) override;

  IRNode *loadVirtFunc(IRNode *Arg1, CORINFO_RESOLVED_TOKEN *ResolvedToken,
                       CORINFO_CALL_INFO *CallInfo, IRNode **NewIR) override {
    throw NotYetImplementedException("loadVirtFunc");
  };
  IRNode *loadPrimitiveType(IRNode *Addr, CorInfoType CorInfoType,
                            ReaderAlignType Alignment, bool IsVolatile,
                            bool IsInterfConst, IRNode **NewIR) override;

  IRNode *loadNonPrimitiveObj(IRNode *Addr, CORINFO_CLASS_HANDLE ClassHandle,
                              ReaderAlignType Alignment, bool IsVolatile,
                              IRNode **NewIR) override {
    throw NotYetImplementedException("loadNonPrimitiveObj");
  };
  IRNode *makeRefAny(CORINFO_RESOLVED_TOKEN *ResolvedToken, IRNode *Object,
                     IRNode **NewIR) override {
    throw NotYetImplementedException("makeRefAny");
  };
  IRNode *newArr(CORINFO_RESOLVED_TOKEN *ResolvedToken, IRNode *Arg1,
                 IRNode **NewIR) override;
  IRNode *newObj(mdToken Token, mdToken LoadFtnToken, uint32_t CurrOffset,
                 IRNode **NewIR) override;
  void pop(IRNode *Opr, IRNode **NewIR) override;
  IRNode *refAnyType(IRNode *Arg1, IRNode **NewIR) override {
    throw NotYetImplementedException("refAnyType");
  };

  void rethrow(IRNode **NewIR) override {
    throw NotYetImplementedException("rethrow");
  };
  void returnOpcode(IRNode *Opr, bool SynchronousMethod,
                    IRNode **NewIR) override;
  IRNode *shift(ReaderBaseNS::ShiftOpcode Opcode, IRNode *ShiftAmount,
                IRNode *ShiftOperand, IRNode **NewIR) override;
  IRNode *sizeofOpcode(CORINFO_RESOLVED_TOKEN *ResolvedToken,
                       IRNode **NewIR) override;
  void storeArg(uint32_t ArgOrdinal, IRNode *Arg1,
                ReaderAlignType Alignment, bool IsVolatile,
                IRNode **NewIR) override;
  void storeElem(ReaderBaseNS::StElemOpcode Opcode,
                 CORINFO_RESOLVED_TOKEN *ResolvedToken, IRNode *Arg1,
                 IRNode *Arg2, IRNode *Arg3, IRNode **NewIR) override {
    throw NotYetImplementedException("storeElem");
  };

  void storeField(CORINFO_RESOLVED_TOKEN *ResolvedToken, IRNode *Arg1,
                  IRNode *Arg2, ReaderAlignType Alignment, bool IsVolatile,
                  IRNode **NewIR) override;

  void storePrimitiveType(IRNode *Value, IRNode *Addr, CorInfoType CorInfoType,
                          ReaderAlignType Alignment, bool IsVolatile,
                          IRNode **NewIR) override {
    throw NotYetImplementedException("storePrimitiveType");
  };
  void storeLocal(uint32_t LocOrdinal, IRNode *Arg1,
                  ReaderAlignType Alignment, bool IsVolatile,
                  IRNode **NewIR) override;
  void storeStaticField(CORINFO_RESOLVED_TOKEN *ResolvedToken, IRNode *Arg1,
                        bool IsVolatile, IRNode **NewIR) override {
    throw NotYetImplementedException("storeStaticField");
  };
  IRNode *stringGetChar(IRNode *Arg1, IRNode *Arg2, IRNode **NewIR) override;

  bool sqrt(IRNode *Arg1, IRNode **RetVal, IRNode **NewIR) override {
    throw NotYetImplementedException("sqrt");
  };

  // The callTarget node is only required on IA64.
  bool interlockedIntrinsicBinOp(IRNode *Arg1, IRNode *Arg2, IRNode **RetVal,
                                 CorInfoIntrinsics IntrinsicID,
                                 IRNode **NewIR) override {
    throw NotYetImplementedException("interlockedIntrinsicBinOp");
  };
  bool interlockedCmpXchg(IRNode *Arg1, IRNode *Arg2, IRNode *Arg3,
                          IRNode **RetVal, CorInfoIntrinsics IntrinsicID,
                          IRNode **NewIR) override {
    throw NotYetImplementedException("interlockedCmpXchg");
  };
  bool memoryBarrier(IRNode **NewIR) override {
    throw NotYetImplementedException("memoryBarrier");
  };
  void switchOpcode(IRNode *Opr, IRNode **NewIR) override {
    throw NotYetImplementedException("switchOpcode");
  };
  void throwOpcode(IRNode *Arg1, IRNode **NewIR) override;
  IRNode *unaryOp(ReaderBaseNS::UnaryOpcode Opcode, IRNode *Arg1,
                  IRNode **NewIR) override;
  IRNode *unbox(CORINFO_RESOLVED_TOKEN *ResolvedToken, IRNode *Arg2,
                IRNode **NewIR, bool AndLoad, ReaderAlignType Alignment,
                bool IsVolatile) override {
    throw NotYetImplementedException("unbox");
  };

  void nop(IRNode **NewIR) override;

  void insertIBCAnnotations() override;
  IRNode *insertIBCAnnotation(FlowGraphNode *Node, uint32_t Count,
                              uint32_t Offset) override {
    throw NotYetImplementedException("insertIBCAnnotation");
  };

  //
  // REQUIRED Client Helper Routines.
  //

  // Base calls to alert client it needs a security check
  void methodNeedsSecurityCheck() override {
    throw NotYetImplementedException("methodNeedsSecurityCheck");
  };

  // Base calls to alert client it needs keep generics context alive
  void
  methodNeedsToKeepAliveGenericsContext(bool KeepGenericsCtxtAlive) override;

  // Called to instantiate an empty reader stack.
  ReaderStack *createStack(uint32_t MaxStack, ReaderBase *Reader) override;

  // Called when reader begins processing method.
  void readerPrePass(uint8_t *Buf, uint32_t NumBytes) override;

  // Called between building the flow graph and inserting the IR
  void readerMiddlePass(void) override;

  // Called when reader has finished processing method.
  void readerPostPass(bool IsImportOnly) override;

  // Called at the start of block processing
  void beginFlowGraphNode(FlowGraphNode *Fg, uint32_t CurrOffset,
                          bool IsVerifyOnly) override;
  // Called at the end of block processing.
  void endFlowGraphNode(FlowGraphNode *Fg, uint32_t CurrOffset,
                        IRNode **NewIR) override;

  // Used to maintain operand stack.
  void maintainOperandStack(IRNode **Opr1, IRNode **Opr2,
                            IRNode **NewIR) override;
  void assignToSuccessorStackNode(FlowGraphNode *, IRNode *Dst, IRNode *Src,
                                  IRNode **NewIR,
                                  bool *IsMultiByteAssign) override {
    throw NotYetImplementedException("assignToSuccessorStackNode");
  };
  //    ReaderStackNode* CopyStackList(ReaderStackNode* stack) =
  //    0;
  bool typesCompatible(IRNode *Src1, IRNode *Src2) override {
    throw NotYetImplementedException("typesCompatible");
  };

  void removeStackInterference(IRNode **NewIR) override;

  void removeStackInterferenceForLocalStore(uint32_t Opcode,
                                            uint32_t Ordinal,
                                            IRNode **NewIR) override;

  // Remove all IRNodes from block (for verification error processing.)
  void clearCurrentBlock(IRNode **NewIR) override {
    throw NotYetImplementedException("clearCurrentBlock");
  };

  // Notify client of alignment problem
  void verifyStaticAlignment(void *Pointer, CorInfoType CorType,
                             unsigned MinClassAlign) override;

  // Allocate temporary (Reader lifetime) memory
  void *getTempMemory(size_t NumBytes) override;
  // Allocate procedure-lifetime memory
  void *getProcMemory(size_t NumBytes) override;

  EHRegion *rgnAllocateRegion() override;
  EHRegionList *rgnAllocateRegionList() override;

  //
  // REQUIRED Flow and Region Graph Manipulation Routines
  //
  FlowGraphNode *fgPrePhase(FlowGraphNode *Fg) override;
  void fgPostPhase(void) override;
  FlowGraphNode *fgGetHeadBlock(void) override;
  FlowGraphNode *fgGetTailBlock(void) override;
  unsigned fgGetBlockCount(void) override;
  FlowGraphNode *fgNodeGetIDom(FlowGraphNode *Fg) override;

  IRNode *fgNodeFindStartLabel(FlowGraphNode *Block) override;

  BranchList *fgGetLabelBranchList(IRNode *LabelNode) override {
    throw NotYetImplementedException("fgGetLabelBranchList");
  };

  void insertHandlerAnnotation(EHRegion *HandlerRegion) override {
    throw NotYetImplementedException("insertHandlerAnnotation");
  };
  void insertRegionAnnotation(IRNode *RegionStartNode,
                              IRNode *RegionEndNode) override {
    throw NotYetImplementedException("insertRegionAnnotation");
  };
  void fgAddLabelToBranchList(IRNode *LabelNode, IRNode *BranchNode) override;
  void fgAddArc(IRNode *BranchNode, FlowGraphNode *Source,
                FlowGraphNode *Sink) override {
    throw NotYetImplementedException("fgAddArc");
  };
  bool fgBlockHasFallThrough(FlowGraphNode *Block) override;

  bool fgBlockIsRegionEnd(FlowGraphNode *Block) override {
    throw NotYetImplementedException("fgBlockIsRegionEnd");
  };
  void fgDeleteBlock(FlowGraphNode *Block) override;
  void fgDeleteEdge(FlowGraphEdgeList *Arc) override {
    throw NotYetImplementedException("fgDeleteEdge");
  };
  void fgDeleteNodesFromBlock(FlowGraphNode *Block) override;

  bool commonTailCallChecks(CORINFO_METHOD_HANDLE DeclaredMethod,
                            CORINFO_METHOD_HANDLE ExactMethod,
                            bool IsUnmarkedTailCall, bool SuppressMsgs);

  // Returns true iff client considers the JMP recursive and wants a
  // loop back-edge rather than a forward edge to the exit label.
  bool fgOptRecurse(mdToken Token) override;

  // Returns true iff client considers the CALL/JMP recursive and wants a
  // loop back-edge rather than a forward edge to the exit label.
  bool fgOptRecurse(ReaderCallTargetData *Data) override;

  // Returns true if node (the start of a new eh Region) cannot be the start of
  // a block.
  bool fgEHRegionStartRequiresBlockSplit(IRNode *Node) override {
    throw NotYetImplementedException("fgEHRegionStartRequiresBlockSplit");
  };

  bool fgIsExceptRegionStartNode(IRNode *Node) override {
    throw NotYetImplementedException("fgIsExceptRegionStartNode");
  };
  FlowGraphNode *fgSplitBlock(FlowGraphNode *Block, IRNode *Node) override;
  void fgSetBlockToRegion(FlowGraphNode *Block, EHRegion *Region,
                          uint32_t LastOffset) override {
    throw NotYetImplementedException("fgSetBlockToRegion");
  };
  IRNode *fgMakeBranch(IRNode *LabelNode, IRNode *InsertNode,
                       uint32_t CurrentOffset, bool IsConditional,
                       bool IsNominal) override;
  IRNode *fgMakeEndFinally(IRNode *InsertNode, uint32_t CurrentOffset,
                           bool IsLexicalEnd) override;

  // turns an unconditional branch to the entry label into a fall-through
  // or a branch to the exit label, depending on whether it was a recursive
  // jmp or tail.call.
  void fgRevertRecursiveBranch(IRNode *BranchNode) override {
    throw NotYetImplementedException("fgRevertRecursiveBranch");
  };

  IRNode *fgMakeSwitch(IRNode *DefaultLabel, IRNode *Insert) override {
    throw NotYetImplementedException("fgMakeSwitch");
  };
  IRNode *fgMakeThrow(IRNode *Insert) override;
  IRNode *fgMakeRethrow(IRNode *Insert) override {
    throw NotYetImplementedException("fgMakeRethrow");
  };
  IRNode *fgAddCaseToCaseList(IRNode *SwitchNode, IRNode *LabelNode,
                              unsigned Element) override {
    throw NotYetImplementedException("fgAddCaseToCaseList");
  };
  void insertEHAnnotationNode(IRNode *InsertionPointNode,
                              IRNode *InsertNode) override {
    throw NotYetImplementedException("insertEHAnnotationNode");
  };
  FlowGraphNode *makeFlowGraphNode(uint32_t TargetOffset,
                                   EHRegion *Region) override;
  void markAsEHLabel(IRNode *LabelNode) override {
    throw NotYetImplementedException("markAsEHLabel");
  };
  IRNode *makeTryEndNode(void) override {
    throw NotYetImplementedException("makeTryEndNode");
  };
  IRNode *makeRegionStartNode(ReaderBaseNS::RegionKind RegionType) override {
    throw NotYetImplementedException("makeRegionStartNode");
  };
  IRNode *makeRegionEndNode(ReaderBaseNS::RegionKind RegionType) override {
    throw NotYetImplementedException("makeRegionEndNode");
  };

  // Allow client to override reader's decision to optimize castclass/isinst
  bool disableCastClassOptimization();

  // Hook to permit client to record call information returns true if the call
  // is a recursive tail
  // call and thus should be turned into a loop
  bool fgCall(ReaderBaseNS::OPCODE Opcode, mdToken Token,
              mdToken ConstraintToken, unsigned MsilOffset, IRNode *Block,
              bool CanInline, bool IsTailCall, bool IsUnmarkedTailCall,
              bool IsReadOnly) override;

  // Hook to permit client to record when calls feed branches
  void fgCmp(ReaderBaseNS::OPCODE) override {
    throw NotYetImplementedException("fgCmp");
  };

  // Replace all uses of oldNode in the IR with newNode and delete oldNode.
  void replaceFlowGraphNodeUses(FlowGraphNode *OldNode,
                                FlowGraphNode *NewNode) override;

  IRNode *findBlockSplitPointAfterNode(IRNode *Node) override;
  IRNode *exitLabel(void) override {
    throw NotYetImplementedException("exitLabel");
  };
  IRNode *entryLabel(void) override {
    throw NotYetImplementedException("entryLabel");
  };

  // Function is passed a try region, and is expected to return the first label
  // or instruction
  // after the region.
  IRNode *findTryRegionEndOfClauses(EHRegion *TryRegion) override {
    throw NotYetImplementedException("findTryRegionEndOfClauses");
  };

  bool isCall(IRNode **NewIR) override {
    throw NotYetImplementedException("isCall");
  };
  bool isRegionStartBlock(FlowGraphNode *Fg) override {
    throw NotYetImplementedException("isRegionStartBlock");
  };
  bool isRegionEndBlock(FlowGraphNode *Fg) override {
    throw NotYetImplementedException("isRegionEndBlock");
  };

  // Create a symbol node that will be used to represent the stack-incoming
  // exception object
  // upon entry to funclets.
  IRNode *makeExceptionObject(IRNode **NewIR) override {
    throw NotYetImplementedException("makeExceptionObject");
  };

  // //////////////////////////////////////////////////////////////////////////
  // Client Supplied Helper Routines, required by VOS support
  // //////////////////////////////////////////////////////////////////////////

  // Asks GenIR to make operand value accessible by address, and return a node
  // that references the incoming operand by address.
  IRNode *addressOfLeaf(IRNode *Leaf, IRNode **NewIR) override;
  IRNode *addressOfValue(IRNode *Leaf, IRNode **NewIR) override;

  // Helper callback used by rdrCall to emit call code.
  IRNode *genCall(ReaderCallTargetData *CallTargetInfo, CallArgTriple *ArgArray,
                  uint32_t NumArgs, IRNode **CallNode,
                  IRNode **NewIR) override;

  bool canMakeDirectCall(ReaderCallTargetData *CallTargetData) override;

  // Generate call to helper
  IRNode *callHelper(CorInfoHelpFunc HelperID, IRNode *Dst, IRNode **NewIR,
                     IRNode *Arg1 = NULL, IRNode *Arg2 = NULL,
                     IRNode *Arg3 = NULL, IRNode *Arg4 = NULL,
                     ReaderAlignType Alignment = Reader_AlignUnknown,
                     bool IsVolatile = false, bool NoCtor = false,
                     bool CanMoveUp = false) override;

  // Generate call to helper
  IRNode *callHelper(CorInfoHelpFunc HelperID, llvm::Type *ReturnType,
                     IRNode **NewIR, IRNode *Arg1 = NULL, IRNode *Arg2 = NULL,
                     IRNode *Arg3 = NULL, IRNode *Arg4 = NULL,
                     ReaderAlignType Alignment = Reader_AlignUnknown,
                     bool IsVolatile = false, bool NoCtor = false,
                     bool CanMoveUp = false);

  // Generate special generics helper that might need to insert flow
  IRNode *callRuntimeHandleHelper(CorInfoHelpFunc Helper, IRNode *Arg1,
                                  IRNode *Arg2, IRNode *NullCheckArg,
                                  IRNode **NewIR) override {
    throw NotYetImplementedException("callRuntimeHandleHelper");
  };

  IRNode *convertToHelperArgumentType(IRNode *Opr, uint32_t DestinationSize,
                                      IRNode **NewIR) override {
    throw NotYetImplementedException("convertToHelperArgumentType");
  };

  IRNode *genNullCheck(IRNode *Node, IRNode **NewIR) override {
    throw NotYetImplementedException("genNullCheck");
  };
  void
  createSym(uint32_t Num, bool IsAuto, CorInfoType CorType,
            CORINFO_CLASS_HANDLE Class, bool IsPinned,
            ReaderSpecialSymbolType SymType = Reader_NotSpecialSymbol) override;

  IRNode *derefAddress(IRNode *Address, bool DstIsGCPtr, bool IsConst,
                       IRNode **NewIR) override {
    throw NotYetImplementedException("derefAddress");
  };

  IRNode *conditionalDerefAddress(IRNode *Address, IRNode **NewIR) override {
    throw NotYetImplementedException("conditionalDerefAddress");
  };

  IRNode *getHelperCallAddress(CorInfoHelpFunc HelperId,
                               IRNode **NewIR) override;

  IRNode *handleToIRNode(mdToken Token, void *EmbedHandle, void *RealHandle,
                         bool IsIndirect, bool IsReadOnly, bool IsRelocatable,
                         bool IsCallTarget, IRNode **NewIR,
                         bool IsFrozenObject = false) override;

  // Create an operand that will be used to hold a pointer.
  IRNode *makePtrDstGCOperand(bool IsInteriorGC) override {
    throw NotYetImplementedException("makePtrDstGCOperand");
  };
  IRNode *makePtrNode(ReaderPtrType PtrType = Reader_PtrNotGc) override;

  IRNode *makeStackTypeNode(IRNode *Node) override {
    throw NotYetImplementedException("makeStackTypeNode");
  };

  IRNode *makeCallReturnNode(CORINFO_SIG_INFO *Sig, unsigned *HiddenMBParamSize,
                             GCLayoutStruct **GcLayout) override;

  IRNode *makeDirectCallTargetNode(CORINFO_METHOD_HANDLE Method,
                                   void *CodeAddr) override;

  // Called once region tree has been built.
  void setEHInfo(EHRegion *EhRegionTree, EHRegionList *EhRegionList) override;

  void setSequencePoint(uint32_t, ICorDebugInfo::SourceTypes,
                        IRNode **NewIR) override {
    throw NotYetImplementedException("setSequencePoint");
  };
  bool needSequencePoints() override;

#if !defined(CC_PEVERIFY)
  //
  // Helper functions for calls
  //

  // vararg
  bool callIsCorVarArgs(IRNode *CallNode);
  void canonVarargsCall(IRNode *CallNode, ReaderCallTargetData *CallTargetInfo,
                        IRNode **NewIR) {
    throw NotYetImplementedException("canonVarargsCall");
  };

  // newobj
  bool canonNewObjCall(IRNode *CallNode, ReaderCallTargetData *CallTargetData,
                       IRNode **OutResult, IRNode **NewIR);
  void canonNewArrayCall(IRNode *CallNode, ReaderCallTargetData *CallTargetData,
                         IRNode **OutResult, IRNode **NewIR);
#endif

  // Used to expand multidimensional array access intrinsics
  bool arrayGet(CORINFO_SIG_INFO *Sig, IRNode **RetVal,
                IRNode **NewIR) override {
    throw NotYetImplementedException("arrayGet");
  };
  bool arraySet(CORINFO_SIG_INFO *Sig, IRNode **NewIR) override {
    throw NotYetImplementedException("arraySet");
  };

#if !defined(NDEBUG)
  void dbDumpFunction(void) override {
    throw NotYetImplementedException("dbDumpFunction");
  };
  void dbPrintIRNode(IRNode *NewIR) override { NewIR->dump(); };
  void dbPrintFGNode(FlowGraphNode *Fg) override {
    throw NotYetImplementedException("dbPrintFGNode");
  };
  void dbPrintEHRegion(EHRegion *Eh) override {
    throw NotYetImplementedException("dbPrintEHRegion");
  };
  uint32_t dbGetFuncHash(void) override {
    throw NotYetImplementedException("dbGetFuncHash");
  };
#endif

private:
  llvm::Type *getType(CorInfoType Type, CORINFO_CLASS_HANDLE ClassHandle,
                      bool GetRefClassFields = true);

  llvm::Function *getFunction(CORINFO_METHOD_HANDLE Method);

  llvm::FunctionType *getFunctionType(CORINFO_METHOD_HANDLE Method);

  llvm::Type *getClassType(CORINFO_CLASS_HANDLE ClassHandle,
                           bool IsRefClass, bool GetRefClassFields);

  /// Convert node to the desired type.
  /// May reinterpret, truncate, or extend as needed.
  /// \p Type - Desired type
  /// \p Node - Value to be converted
  /// \p IsSigned - Perform sign extension if necessary, otherwise
  ///               integral values are zero extended
  IRNode *convert(llvm::Type *Type, llvm::Value *Node, bool IsSigned);

  llvm::Type *binaryOpType(llvm::Type *Type1, llvm::Type *Type2);

  IRNode *genPointerAdd(IRNode *Arg1, IRNode *Arg2);
  IRNode *genPointerSub(IRNode *Arg1, IRNode *Arg2);
  IRNode *getFieldAddress(CORINFO_RESOLVED_TOKEN *ResolvedToken,
                          CORINFO_FIELD_INFO *FieldInfo, IRNode *Obj,
                          bool MustNullCheck, IRNode **NewIR);

  IRNode *simpleFieldAddress(IRNode *BaseAddress,
                             CORINFO_RESOLVED_TOKEN *ResolvedToken,
                             CORINFO_FIELD_INFO *FieldInfo,
                             IRNode **NewIR) override;

  void classifyCmpType(llvm::Type *Ty, uint32_t &Size, bool &IsPointer,
                       bool &IsFloat);

  uint32_t size(CorInfoType CorType);
  uint32_t stackSize(CorInfoType CorType);
  static bool isSigned(CorInfoType CorType);
  llvm::Type *getStackType(CorInfoType CorType);

  /// Convert a result to a valid stack type,
  /// generally by either reinterpretation or extension.
  /// \p Node - Value to be converted
  /// \p CorType - additional information needed to determine if
  ///              Node's type is signed or unsigned
  IRNode *convertToStackType(IRNode *Node, CorInfoType CorType);

  /// Convert a result from a stack type to the desired type,
  /// generally by either reinterpretation or truncation.
  /// \p Node - Value to be converted
  /// \p CorType - additional information needed to determine if
  ///              ResultTy is signed or unsigned
  /// \p ResultTy - Desired type
  IRNode *convertFromStackType(IRNode *Node, CorInfoType CorType,
                               llvm::Type *ResultTy);

  bool objIsThis(IRNode *Obj);

  /// Create a new temporary variable that can be
  /// used anywhere within the method.
  ///
  /// \p Ty - Type for the new variable.
  /// \returns Instruction establishing the variable's location.
  llvm::Instruction * createTemporary(llvm::Type * Ty);

  IRNode *
  loadManagedAddress(const std::vector<llvm::Value *> &UnmanagedAddresses,
                     uint32_t Index);

  llvm::PointerType *getManagedPointerType(llvm::Type *ElementType);

  llvm::PointerType *getUnmanagedPointerType(llvm::Type *ElementType);

  bool isManagedPointerType(llvm::PointerType *PointerType);

  uint32_t argOrdinalToArgIndex(uint32_t ArgOrdinal);
  uint32_t argIndexToArgOrdinal(uint32_t ArgIndex);

  void makeStore(llvm::Type *Ty, llvm::Value *Address,
                 llvm::Value *ValueToStore, bool IsVolatile, IRNode **NewIR);

private:
  MSILCJitContext *JitContext;
  llvm::Function *Function;
  llvm::IRBuilder<> *LLVMBuilder;
  std::map<CORINFO_CLASS_HANDLE, llvm::Type *> *ClassTypeMap;
  std::map<std::tuple<CorInfoType, CORINFO_CLASS_HANDLE, uint32_t>,
           llvm::Type *> *ArrayTypeMap;
  std::map<CORINFO_FIELD_HANDLE, uint32_t> *FieldIndexMap;
  std::map<llvm::BasicBlock *, FlowGraphNodeInfo> FlowGraphInfoMap;
  std::vector<llvm::Value *> LocalVars;
  std::vector<CorInfoType> LocalVarCorTypes;
  std::vector<llvm::Value *> Arguments;
  std::vector<CorInfoType> ArgumentCorTypes;
  CorInfoType ReturnCorType;
  bool HasThis;
  bool HasTypeParameter;
  bool HasVarargsToken;
  llvm::BasicBlock *EntryBlock;
  llvm::Instruction *TempInsertionPoint;
  uint32_t TargetPointerSizeInBits;
  const uint32_t UnmanagedAddressSpace = 0;
  const uint32_t ManagedAddressSpace = 1;
};

#endif // MSIL_READER_IR_H
