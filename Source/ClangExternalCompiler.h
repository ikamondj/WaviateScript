#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "AbstractCompiler.h"

#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/PreprocessorOptions.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/TargetSelect.h>

template <bool cppMode>
class ClangCompiler final : public AbstractCompiler {
public:
    ClangCompiler();
    ~ClangCompiler() = default;

    void compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency) override;

    const void* getDispatchPtr() const noexcept {
        return dispatch_.load(std::memory_order_acquire);
    }

private:
    struct Dispatch final {
        SampleShader sample = nullptr;
        FrequencyShader freq = nullptr;
    };

    struct CompiledUnit final {
        std::unique_ptr<llvm::LLVMContext> ctx;
        std::unique_ptr<llvm::ExecutionEngine> ee;
        Dispatch dispatch{};
    };

    static constexpr size_t kKeepOldUnits = 4;

    std::unique_ptr<CompiledUnit> active_;
    std::deque<std::unique_ptr<CompiledUnit>> retired_;
    std::atomic<const Dispatch*> dispatch_{ nullptr };

    static std::string buildTranslationUnit(const std::string& userSource);
    static void configureInvocation(std::shared_ptr <clang::CompilerInvocation>& inv, const char* virtualFilename);
    static std::unique_ptr<llvm::Module> emitLLVMModule(
        llvm::LLVMContext& ctx,
        std::unique_ptr<llvm::MemoryBuffer> buffer
    );

    static std::unique_ptr<llvm::ExecutionEngine> buildJIT(std::unique_ptr<llvm::Module> m);
    void retireOldActive();
};

template <bool cppMode>
ClangCompiler<cppMode>::ClangCompiler() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

template <bool cppMode>
std::string ClangCompiler<cppMode>::buildTranslationUnit(const std::string& userSource) {
    std::string tu;
    tu.reserve(userSource.size() + 512);

    if constexpr (cppMode) {
        tu.append(R"(#include "C:\Program Files\Waviate\Script\Include\Waviate.hpp")");
        tu.append("\n\n");
        tu.append(userSource);
        tu.append("\n\n");
        tu.append(R"(#include "C:\Program Files\Waviate\Script\Include\WaviateCppShim.hpp")");
        tu.append("\n");
    }
    else {
        tu.append(R"(#include "C:\Program Files\Waviate\Script\Include\Waviate.h")");
        tu.append("\n\n");
        tu.append(userSource);
        tu.append("\n");
    }

    return tu;
}

template <bool cppMode>
void ClangCompiler<cppMode>::configureInvocation(std::shared_ptr<clang::CompilerInvocation>& inv, const char* virtualFilename) {
    inv = std::make_shared<clang::CompilerInvocation>();
    inv->getTargetOpts().Triple = llvm::sys::getDefaultTargetTriple();

    auto& fe = inv->getFrontendOpts();
    fe.Inputs.clear();

    clang::InputKind kind = cppMode
        ? clang::InputKind(clang::Language::CXX)
        : clang::InputKind(clang::Language::C);

    fe.Inputs.emplace_back(virtualFilename, kind);
    fe.DisableFree = false;

    auto& lang = inv->getLangOpts();
    if constexpr (cppMode) {
        lang.CPlusPlus = true;
        lang.CPlusPlus17 = true;
    }
    else {
        lang.C99 = true;
    }

    auto& cg = inv->getCodeGenOpts();
    cg.OptimizationLevel = 2;

    auto& hs = inv->getHeaderSearchOpts();
    hs.UseBuiltinIncludes = true;
    hs.UseStandardSystemIncludes = false;
    hs.UseStandardCXXIncludes = false;

    hs.AddPath(R"(C:\Program Files\Waviate\Script\Include)", clang::frontend::System, false, false);
}

template <bool cppMode> 
std::unique_ptr<llvm::Module> ClangCompiler<cppMode>::emitLLVMModule(
    llvm::LLVMContext& ctx,
    std::unique_ptr<llvm::MemoryBuffer> buffer
) {
    auto diagOpts = llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions>(new clang::DiagnosticOptions());
    clang::TextDiagnosticPrinter diagClient(llvm::errs(), diagOpts.get());

    clang::CompilerInstance ci;
    std::shared_ptr<clang::CompilerInvocation> invNew;
    const char* virtualFilename = cppMode ? "shader.cpp" : "shader.c";
    configureInvocation(invNew, virtualFilename);
    ci.setInvocation(invNew);
    ci.createDiagnostics(&diagClient, false);
    if (!ci.hasDiagnostics()) return nullptr;

    ci.setTarget(clang::TargetInfo::CreateTargetInfo(ci.getDiagnostics(), ci.getInvocation().TargetOpts));
    if (!ci.hasTarget()) return nullptr;

    const auto& inputs = invNew->getFrontendOpts().Inputs;
    if (inputs.empty()) return nullptr;

    const std::string vFilename = inputs.front().getFile().str();

    ci.createFileManager();
    ci.createSourceManager(ci.getFileManager());

    ci.getPreprocessorOpts().RetainRemappedFileBuffers = true;
    ci.getPreprocessorOpts().addRemappedFile(vFilename, buffer.release());

    clang::EmitLLVMOnlyAction action(&ctx);
    if (!ci.ExecuteAction(action)) return nullptr;

    return action.takeModule();
}

template <bool cppMode>
std::unique_ptr<llvm::ExecutionEngine> ClangCompiler<cppMode>::buildJIT(std::unique_ptr<llvm::Module> m) {
    if (!m) return nullptr;

    std::string err;
    llvm::ExecutionEngine* raw = llvm::EngineBuilder(std::move(m))
        .setEngineKind(llvm::EngineKind::JIT)
        .setErrorStr(&err)
        .create();

    if (!raw) return nullptr;

    return std::unique_ptr<llvm::ExecutionEngine>(raw);
}

template <bool cppMode>
void ClangCompiler<cppMode>::retireOldActive() {
    if (active_) {
        retired_.push_back(std::move(active_));
        while (retired_.size() > kKeepOldUnits) {
            retired_.pop_front();
        }
    }
}

template <bool cppMode>
void ClangCompiler<cppMode>::compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency) {
    outSample = nullptr;
    outFrequency = nullptr;

    dispatch_.store(nullptr, std::memory_order_release);

    const char* virtualFilename = cppMode ? "shader.cpp" : "shader.c";
    const std::string tu = buildTranslationUnit(source);

    auto unit = std::make_unique<CompiledUnit>();
    unit->ctx = std::make_unique<llvm::LLVMContext>();

    auto buffer = llvm::MemoryBuffer::getMemBufferCopy(tu, virtualFilename);
    if (!buffer) return;


    auto module = emitLLVMModule(*unit->ctx, std::move(buffer));
    if (!module) return;

    unit->ee = buildJIT(std::move(module));
    if (!unit->ee) return;

    unit->ee->finalizeObject();

    const uint64_t sampleAddr = unit->ee->getFunctionAddress("sample_process");
    const uint64_t freqAddr = unit->ee->getFunctionAddress("frequency_process");

    unit->dispatch.sample = sampleAddr ? reinterpret_cast<SampleShader>(sampleAddr) : nullptr;
    unit->dispatch.freq = freqAddr ? reinterpret_cast<FrequencyShader>(freqAddr) : nullptr;

    outSample = unit->dispatch.sample;
    outFrequency = unit->dispatch.freq;

    retireOldActive();
    active_ = std::move(unit);

    dispatch_.store(&active_->dispatch, std::memory_order_release);
}