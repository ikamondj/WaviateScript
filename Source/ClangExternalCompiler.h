#pragma once

#include <JuceHeader.h>
#include <memory>
#include "AbstractCompiler.h"

#include <clang/AST/Type.h>
#include <clang/AST/ASTContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Function.h>
#include "llvm/IR/ModuleSummaryIndex.h"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
//todo include llvm BackendUtil.h

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Basic/LLVM.h>


#include <sstream>

template <bool cppMode>
class ClangCompiler : public AbstractCompiler {
public:
    void compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency) override;
    ClangCompiler();
    ~ClangCompiler();
private:
    std::unique_ptr<llvm::ExecutionEngine> executionEngine;
    std::unique_ptr<llvm::Module> currentModule;
};

// Template implementation
template <bool cppMode>
ClangCompiler<cppMode>::ClangCompiler()
{
    // Initialize LLVM JIT infrastructure
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

template <bool cppMode>
ClangCompiler<cppMode>::~ClangCompiler() = default;

template <bool cppMode>
void ClangCompiler<cppMode>::compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency) {
    // Initialize output to nullptr
    outSample = nullptr;
    outFrequency = nullptr;

    try {
        // Prepend include directive
        std::string sourceWithInclude = (cppMode ? R"(
#include "WaviateInput.hpp"

)" : R"(
#include "WaviateInput.h"

)") + source;

        // Create LLVM context
        auto LLVMCtx = std::make_unique<llvm::LLVMContext>();

        // Setup Clang diagnostics
        auto DiagOpts = llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions>(new clang::DiagnosticOptions());
        clang::TextDiagnosticPrinter DiagClient(llvm::errs(), DiagOpts.get());
        auto DiagID = llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs>(new clang::DiagnosticIDs());
        clang::DiagnosticsEngine Diags(DiagID, DiagOpts, &DiagClient);

        // Create and configure compiler invocation
        auto Invocation = std::make_shared<clang::CompilerInvocation>();

        // Setup basic compiler options
        clang::CompilerInvocation::CreateFromArgs(
            *Invocation,
            {},
            Diags
        );

        // Set language and mode
        if (cppMode) {
            Invocation->getLangOpts().CPlusPlus = true;
        }
        Invocation->getCodeGenOpts().OptimizationLevel = 2;

        // Setup include paths to find WaviateInput.h
        auto& HeaderOpts = Invocation->getHeaderSearchOpts();
        HeaderOpts.AddPath("Source", clang::frontend::System, false, false);
        HeaderOpts.AddPath(".", clang::frontend::System, false, false);

        // Create compiler instance
        clang::CompilerInstance Compiler;
        Compiler.setInvocation(Invocation);
        Compiler.createDiagnostics(&DiagClient, false);

        // Set target
        if (!Compiler.hasTarget()) {
            if (!Compiler.createTarget()) {
                return;
            }
        }

        // Create memory buffer from source code
        llvm::StringRef SourceCode(sourceWithInclude);
        auto Buffer = llvm::MemoryBuffer::getMemBufferCopy(
            SourceCode,
            cppMode ? "shader.cpp" : "shader.c"
        );

        if (!Buffer) {
            return;
        }

        // Create input file
        auto FileEntry = Compiler.getFileManager().getVirtualFileRef(
            cppMode ? "shader.cpp" : "shader.c",
            Buffer->getBufferSize(),
            0
        );

        // Create code gen action with required constructor arguments
        clang::EmitLLVMOnlyAction action(LLVMCtx.get());


        // Perform compilation
        if (!Compiler.ExecuteAction(action)) {
            return;
        }

        // Get the generated LLVM module
        std::unique_ptr<llvm::Module> Module = action.takeModule();
        if (!Module) {
            return;
        }

        // Create execution engine
        std::string ErrStr;
        llvm::ExecutionEngine* EE = llvm::EngineBuilder(std::move(Module))
            .setEngineKind(llvm::EngineKind::JIT)
            .setErrorStr(&ErrStr)
            .create();

        if (!EE) {
            return;
        }

        // Store the execution engine
        executionEngine.reset(EE);

        // Finalize object code for execution
        executionEngine->finalizeObject();

        // Look up and get function addresses
        uint64_t SampleAddr = executionEngine->getFunctionAddress("sample_process");
        if (SampleAddr != 0) {
            outSample = reinterpret_cast<SampleShader>(SampleAddr);
        }

        uint64_t FrequencyAddr = executionEngine->getFunctionAddress("frequency_process");
        if (FrequencyAddr != 0) {
            outFrequency = reinterpret_cast<FrequencyShader>(FrequencyAddr);
        }

    }
    catch (...) {
        // If any exception occurs, ensure outputs are nullptrs
        outSample = nullptr;
        outFrequency = nullptr;
    }
}
