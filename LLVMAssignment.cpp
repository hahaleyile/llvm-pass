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


///!TODO TO BE COMPLETED BY YOU FOR ASSIGNMENT 2
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

    void handleCallInst(const CallInst *callInst) {
        if (callInst->getCalledFunction()) {
            const std::string funcName = callInst->getCalledFunction()->getName().data();
            if (funcName != "llvm.dbg.value") {
                addResult(callInst->getDebugLoc(), funcName);
            }
        } else {
            /// https://www.llvm.org/docs/ProgrammersManual.html#the-value-class
            /// One important aspect of LLVM is that there is no distinction between an SSA variable and the operation that produces it.
            /// Because of this, any reference to the value produced by an instruction
            /// (or the value available as an incoming argument, for example)
            /// is represented as a direct pointer to the instance of the class that represents this value.
            /// Although this may take some getting used to,
            /// it simplifies the representation and makes it easier to manipulate.
            const Value *value = callInst->getCalledOperand();

            if (const PHINode *phiNode = dyn_cast<PHINode>(value)) {
                for (const Value *node: phiNode->incoming_values()) {
                    if (const Function *function = dyn_cast<Function>(node)) {
                        addResult(callInst->getDebugLoc(), function->getName().data());
                    } else {
                        errs() << value->getName().data() << "\n";
                    }
                }
            } else {
                errs() << value->getName().data() << "\n";
            }
        }
    }

    bool runOnModule(Module &M) override {
        errs() << "Hello: ";
        errs().write_escaped(M.getName()) << '\n';
        M.dump();
        errs() << "------------------------------\n";

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

