//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include <llvm/Support/CommandLine.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Instructions.h>
#include <iostream>
#include "llvm/IR/InstIterator.h"
#include <llvm/Support/ToolOutputFile.h>

#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>

#include <llvm/IR/Function.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>


using namespace llvm;
static ManagedStatic<LLVMContext> GlobalContext;

static LLVMContext &getGlobalContext() { return *GlobalContext; }

/* In LLVM 5.0, when  -O0 passed to clang , the functions generated with clang will
 * have optnone attribute which would lead to some transform passes disabled, like mem2reg.
 */
struct EnableFunctionOptPass : public FunctionPass {
    static char ID;

    EnableFunctionOptPass() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
        if (F.hasFnAttribute(Attribute::OptimizeNone)) {
            F.removeFnAttr(Attribute::OptimizeNone);
        }
        return true;
    }
};

char EnableFunctionOptPass::ID = 0;


///Updated 11/10/2017 by fargo: make all functions
///processed by mem2reg before this pass.
struct FuncPtrPass : public ModulePass {
    static char ID; // Pass identification, replacement for typeid
    std::map<const DebugLoc *, std::set<std::string>> Results;

    FuncPtrPass() : ModulePass(ID) {}

    void addResult(const DebugLoc &debugLoc, const std::string &funcName) {
        std::set<std::string> &s = Results[&debugLoc];
        s.insert(funcName);
    }

    void printResult() {
        for (const auto &result: Results) {
            errs() << result.first->getLine() << " : ";
            const auto &funcNames = result.second;
            for (auto iter = funcNames.begin(); iter != funcNames.end(); iter++) {
                if (iter != funcNames.begin()) {
                    errs() << ", ";
                }
                errs() << *iter;
            }
            errs() << "\n";
        }
    }

    void handlePHINode(const PHINode *phiNode, const DebugLoc &debugLoc) {
        for (const Value *node: phiNode->incoming_values()) {
            handleValue(node, debugLoc);
        }
    }

    void handleReturn(const ReturnInst *returnInst, const DebugLoc &debugLoc) {
        handleValue(returnInst->getReturnValue(), debugLoc);
    }

    void handleArgument(const Argument *argument, const DebugLoc &debugLoc) {
        const Function *parentFunc = argument->getParent();
        for (const User *user: parentFunc->users()) {
            if (const CallInst *callInst = dyn_cast<CallInst>(user)) {
                Value *operand = callInst->getArgOperand(argument->getArgNo());
                handleValue(operand, debugLoc);
            } else if (const PHINode *phiNode = dyn_cast<PHINode>(user)) {
                for (const User *phiUser: phiNode->users()) {
                    if (const CallInst *outerCallInst = dyn_cast<CallInst>(phiUser)) {
                        Value *operand = outerCallInst->getArgOperand(argument->getArgNo());
                        handleValue(operand, debugLoc);
                    } else {
                        throw std::exception();
                    }
                }
            } else {
                throw std::exception();
            }
        }
    }

    void handleCall(const CallInst *callInst, const DebugLoc &debugLoc) {
        Function *function = callInst->getCalledFunction();
        if (function) {
            /// 返回值可能不止一个
            for (inst_iterator it = inst_begin(function), et = inst_end(function); it != et; ++it) {
                if (const ReturnInst *returnInst = dyn_cast<ReturnInst>(&*it)) {
                    handleValue(returnInst, debugLoc);
                }
            }
        } else {
            if (const PHINode *phiNode = dyn_cast<PHINode>(callInst->getCalledOperand())) {
                for (Value *value: phiNode->incoming_values()) {
                    if ((function = dyn_cast<Function>(value))) {
                        for (inst_iterator it = inst_begin(function), et = inst_end(function); it != et; ++it) {
                            if (const ReturnInst *returnInst = dyn_cast<ReturnInst>(&*it)) {
                                handleValue(returnInst, debugLoc);
                            }
                        }
                    }
                }
            } else {
                throw std::exception();
            }
        }
    }

    void handleValue(const Value *value, const DebugLoc &debugLoc) {
        if (const PHINode *phiNode = dyn_cast<PHINode>(value)) {
            handlePHINode(phiNode, debugLoc);
        } else if (const Argument *argument = dyn_cast<Argument>(value)) {
            handleArgument(argument, debugLoc);
        } else if (isa<ConstantPointerNull>(value)) {
            return;
        } else if (const Function *function = dyn_cast<Function>(value)) {
            if (strcmp(function->getName().data(), "llvm.dbg.value") != 0) {
                addResult(debugLoc, function->getName().data());
            }
        } else if (const ReturnInst *returnInst = dyn_cast<ReturnInst>(value)) {
            handleReturn(returnInst, debugLoc);
        } else if (const CallInst *callInst = dyn_cast<CallInst>(value)) {
            handleCall(callInst, debugLoc);
        } else {
            auto t = value->getType();
            value->dump();
            throw std::exception();
        }
    }

    void handleCallInst(const CallInst *callInst) {
        /// https://www.llvm.org/docs/ProgrammersManual.html#the-value-class
        /// One important aspect of LLVM is that there is no distinction between an SSA variable and the operation that produces it.
        /// Because of this, any reference to the value produced by an instruction
        /// (or the value available as an incoming argument, for example)
        /// is represented as a direct pointer to the instance of the class that represents this value.
        /// Although this may take some getting used to,
        /// it simplifies the representation and makes it easier to manipulate.
        const Value *value = callInst->getCalledOperand();

        handleValue(value, callInst->getDebugLoc());
    }

    bool runOnModule(Module &M) override {
        for (const Function &f: M) {
            for (const BasicBlock &b: f) {
                for (const Instruction &i: b) {
                    if (const CallInst *callInst = dyn_cast<CallInst>(&i))
                        handleCallInst(callInst);
                }
            }
        }

        printResult();

        return false;
    }
};


char FuncPtrPass::ID = 0;
static RegisterPass<FuncPtrPass> X("funcptrpass", "Print function call instruction");

static cl::opt<std::string>
        InputFilename(cl::Positional,
                      cl::desc("<filename>.bc"),
                      cl::init(""));


int main(int argc, char **argv) {
    const char *c[2];
    std::string s("/home/black/llvm-pass/bc/test");
    if (argc == 1) {
        std::string t;
        std::cout << "请输入测试编号：";
        std::cin >> t;
        s.append(t).append(".bc");
        c[1] = s.c_str();
        // Parse the command line to read the Inputfilename
        cl::ParseCommandLineOptions(2, c,
                                    "FuncPtrPass \n My first LLVM too which does not do much.\n");
    } else {
        // Parse the command line to read the Inputfilename
        cl::ParseCommandLineOptions(argc, argv,
                                    "FuncPtrPass \n My first LLVM too which does not do much.\n");
    }
    LLVMContext &Context = getGlobalContext();
    SMDiagnostic Err;


    // Load the input module
    std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
    if (!M) {
        Err.print(argv[0], errs());
        return 1;
    }

    llvm::legacy::PassManager Passes;

    ///Remove functions' optnone attribute in LLVM5.0
    Passes.add(new EnableFunctionOptPass());
    ///Transform it to SSA
    Passes.add(llvm::createPromoteMemoryToRegisterPass());

    /// Your pass to print Function and Call Instructions
    Passes.add(new FuncPtrPass());
    Passes.run(*M.get());
}

