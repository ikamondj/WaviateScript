#pragma once

#include <atomic>
#include <cctype>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "AbstractCompiler.h"
#include "WaviateCppLanguageModel.h"
#include "WaviateAudio.h"
#include "WaviateSafety.h"
#include "WaviateSafetyValidator.h"
#include "WaviateFuelInstrumentation.h"

#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/LangStandard.h>
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
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/TargetSelect.h>

template <bool cppMode>
class ClangCompiler final : public AbstractCompiler {
public:
    ClangCompiler();
    ~ClangCompiler() = default;

    void compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency) override;

    [[nodiscard]] ShaderRuntimeControls getRuntimeControls() const noexcept override {
        ShaderRuntimeControls controls;
        controls.setFuelBudget = &waviate::safety::setFuelBudget;
        controls.getFuelRemaining = &waviate::safety::getFuelRemaining;
        controls.getFuelExhausted = &waviate::safety::getFuelExhausted;
        return controls;
    }

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

    static bool isIdentifierChar(char c) noexcept;
    static std::string validateSourceCapabilities(const std::string& userSource);
    static std::string stripCommentsAndStrings(const std::string& source);
    static bool containsFunctionLikeIdentifier(const std::string& source, const char* name);
    static std::string buildEmbeddedCppApi();
    static std::string buildCppAbiShim(const std::string& userSource);
    static std::string buildTranslationUnit(const std::string& userSource);
    static void configureInvocation(std::shared_ptr <clang::CompilerInvocation>& inv, const char* virtualFilename);
    static std::unique_ptr<llvm::Module> emitLLVMModule(
        llvm::LLVMContext& ctx,
        std::unique_ptr<llvm::MemoryBuffer> buffer,
        std::string& diagnostics
    );

    static std::unique_ptr<llvm::ExecutionEngine> buildJIT(std::unique_ptr<llvm::Module> m);
    void retireOldActive();
};

template <bool cppMode>
ClangCompiler<cppMode>::ClangCompiler() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    waviate::safety::registerRuntimeSymbols();
}

template <bool cppMode>
bool ClangCompiler<cppMode>::isIdentifierChar(char c) noexcept {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

template <bool cppMode>
static std::string trimShaderLineForValidation(std::string line) {
    while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r' || line.back() == '\n'))
        line.pop_back();

    size_t first = 0;
    while (first < line.size() && (line[first] == ' ' || line[first] == '\t'))
        ++first;

    return line.substr(first);
}

template <bool cppMode>
std::string ClangCompiler<cppMode>::validateSourceCapabilities(const std::string& userSource) {
    const auto stripped = stripCommentsAndStrings(userSource);
    std::ostringstream diagnostics;

    auto reject = [&diagnostics](const std::string& message) {
        diagnostics << "- " << message << '\n';
    };

    std::istringstream lines(stripped);
    std::string line;
    int lineNumber = 1;

    while (std::getline(lines, line)) {
        const auto trimmed = trimShaderLineForValidation<cppMode>(line);

        if (!trimmed.empty() && trimmed.front() == '#') {
            size_t cursor = 1;
            while (cursor < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[cursor])) != 0)
                ++cursor;

            const auto directiveStart = cursor;
            while (cursor < trimmed.size() && isIdentifierChar(trimmed[cursor]))
                ++cursor;

            const auto directive = trimmed.substr(directiveStart, cursor - directiveStart);
            reject("preprocessor directive '#" + directive + "' on line " + std::to_string(lineNumber)
                + " is not allowed in Waviate shader code");
        }

        ++lineNumber;
    }

    static constexpr const char* forbiddenIdentifiers[] = {
        "asm", "__asm", "__asm__", "reinterpret_cast", "const_cast", "dynamic_cast", "typeid",
        "try", "throw", "catch",
        "new", "delete", "malloc", "calloc", "realloc", "free", "alloca", "_alloca", "_malloca",
        "system", "popen", "_popen",
        "CreateProcess", "CreateProcessA", "CreateProcessW", "ShellExecute", "ShellExecuteA", "ShellExecuteW",
        "WinExec", "LoadLibrary", "LoadLibraryA", "LoadLibraryW", "LoadLibraryExA", "LoadLibraryExW",
        "GetProcAddress", "FreeLibrary",
        "dlopen", "dlsym", "dlclose",
        "fork", "vfork", "execl", "execle", "execlp", "execv", "execve", "execvp", "execvpe",
        "spawn", "socket", "connect", "bind", "listen", "accept", "send", "recv",
        "fopen", "freopen", "remove", "rename", "ifstream", "ofstream", "fstream",
        "thread", "mutex", "atomic", "_Atomic", "__atomic", "__c11_atomic", "_Interlocked", "volatile",
        "__has_include", "__has_include_next",
        "waviate_consume_fuel", "waviate_fuel_trap",
        "__waviate_internal_arena_allocate", "__waviate_internal_arena_generation"
    };

    size_t cursor = 0;
    while (cursor < stripped.size()) {
        if (!isIdentifierChar(stripped[cursor])) {
            ++cursor;
            continue;
        }

        const auto start = cursor;
        while (cursor < stripped.size() && isIdentifierChar(stripped[cursor]))
            ++cursor;

        const auto identifier = stripped.substr(start, cursor - start);

        if (identifier.rfind("__builtin_", 0) == 0) {
            reject("compiler builtin '" + identifier + "' is not allowed in user shader code");
            continue;
        }

        for (const auto* forbidden : forbiddenIdentifiers) {
            if (identifier == forbidden) {
                reject("identifier '" + identifier + "' is not allowed in user shader code");
                break;
            }
        }

        if constexpr (cppMode) {
            static constexpr const char* forbiddenCppAbiIdentifiers[] = {
                "sample_process", "frequency_process",
                "WaviateSampleInput", "WaviateFrequencyInput",
                "WaviateSampleStateWriter", "WaviateFrequencyStateWriter"
            };

            for (const auto* forbidden : forbiddenCppAbiIdentifiers) {
                if (identifier == forbidden) {
                    reject("raw ABI identifier '" + identifier
                        + "' is internal; use SampleProcess(WaviateSample&) or FrequencyProcess(WaviateFrequency&)");
                    break;
                }
            }
        }
    }

    return diagnostics.str();
}

template <bool cppMode>
std::string ClangCompiler<cppMode>::stripCommentsAndStrings(const std::string& source) {
    enum class ScanState {
        normal,
        lineComment,
        blockComment,
        stringLiteral,
        charLiteral
    };

    std::string stripped = source;
    ScanState state = ScanState::normal;

    for (size_t i = 0; i < source.size(); ++i) {
        const char c = source[i];
        const char next = (i + 1 < source.size()) ? source[i + 1] : '\0';

        switch (state) {
        case ScanState::normal:
            if (c == '/' && next == '/') {
                stripped[i] = ' ';
                stripped[i + 1] = ' ';
                ++i;
                state = ScanState::lineComment;
            }
            else if (c == '/' && next == '*') {
                stripped[i] = ' ';
                stripped[i + 1] = ' ';
                ++i;
                state = ScanState::blockComment;
            }
            else if (c == '"') {
                stripped[i] = ' ';
                state = ScanState::stringLiteral;
            }
            else if (c == '\'') {
                stripped[i] = ' ';
                state = ScanState::charLiteral;
            }
            break;

        case ScanState::lineComment:
            if (c == '\n' || c == '\r') {
                state = ScanState::normal;
            }
            else {
                stripped[i] = ' ';
            }
            break;

        case ScanState::blockComment:
            stripped[i] = (c == '\n' || c == '\r') ? c : ' ';
            if (c == '*' && next == '/') {
                stripped[i + 1] = ' ';
                ++i;
                state = ScanState::normal;
            }
            break;

        case ScanState::stringLiteral:
            stripped[i] = (c == '\n' || c == '\r') ? c : ' ';
            if (c == '\\' && next != '\0') {
                stripped[i + 1] = (next == '\n' || next == '\r') ? next : ' ';
                ++i;
            }
            else if (c == '"') {
                state = ScanState::normal;
            }
            break;

        case ScanState::charLiteral:
            stripped[i] = (c == '\n' || c == '\r') ? c : ' ';
            if (c == '\\' && next != '\0') {
                stripped[i + 1] = (next == '\n' || next == '\r') ? next : ' ';
                ++i;
            }
            else if (c == '\'') {
                state = ScanState::normal;
            }
            break;
        }
    }

    return stripped;
}

template <bool cppMode>
bool ClangCompiler<cppMode>::containsFunctionLikeIdentifier(const std::string& source, const char* name) {
    const std::string identifier(name);
    size_t pos = 0;

    while ((pos = source.find(identifier, pos)) != std::string::npos) {
        const size_t end = pos + identifier.size();
        const bool leftBoundary = pos == 0 || !isIdentifierChar(source[pos - 1]);
        const bool rightBoundary = end >= source.size() || !isIdentifierChar(source[end]);

        if (leftBoundary && rightBoundary) {
            size_t cursor = end;
            while (cursor < source.size() && std::isspace(static_cast<unsigned char>(source[cursor])) != 0)
                ++cursor;

            if (cursor < source.size() && source[cursor] == '(')
                return true;
        }

        pos = end;
    }

    return false;
}

template <bool cppMode>
std::string ClangCompiler<cppMode>::buildEmbeddedCppApi() {
    return waviate::language::buildEmbeddedCppApi();
}

template <bool cppMode>
std::string ClangCompiler<cppMode>::buildCppAbiShim(const std::string& userSource) {
    const std::string strippedSource = stripCommentsAndStrings(userSource);
    const bool hasCAbiSample = containsFunctionLikeIdentifier(strippedSource, "sample_process");
    const bool hasCAbiFrequency = containsFunctionLikeIdentifier(strippedSource, "frequency_process");
    const bool hasCppSample = containsFunctionLikeIdentifier(strippedSource, "SampleProcess");
    const bool hasCppFrequency = containsFunctionLikeIdentifier(strippedSource, "FrequencyProcess");

    std::string shim;
    if (!hasCAbiSample && hasCppSample) {
        shim.append(R"wslshim(
extern "C" float sample_process(const WaviateSampleInput* input, WaviateSampleStateWriter* writer) {
    WaviateSample wav(input, writer);
    return SampleProcess(wav);
}
)wslshim");
    }

    if (!hasCAbiFrequency && hasCppFrequency) {
        shim.append(R"wslshim(
extern "C" WaviateComplex frequency_process(const WaviateFrequencyInput* input, WaviateFrequencyStateWriter* writer) {
    WaviateFrequency wav(input, writer);
    return FrequencyProcess(wav);
}
)wslshim");
    }

    return shim;
}

inline std::string sanitizeIdentifier(std::string_view name)
{
    std::string result;
    for (char c : name)
    {
        if (std::isalnum(static_cast<unsigned char>(c)))
            result.push_back(c);
        else
            result.push_back('_');
    }
    if (result.empty() || std::isdigit(static_cast<unsigned char>(result[0])))
        result.insert(result.begin(), '_');
    return result;
}

template <bool cppMode>
std::string ClangCompiler<cppMode>::buildTranslationUnit(const std::string& userSource) {
    std::string tu;
    tu.reserve(userSource.size() + 4096);

    if constexpr (cppMode) {
        tu.append("#line 1 \"WaviateSdk.hpp\"\n");
        tu.append(buildEmbeddedCppApi());

        if (auto* cache = waviate::audio::getCurrentThreadAudioCache())
        {
            std::string clipsPrelude = "\nnamespace Clips {\n";
            for (const auto& entry : cache->snapshot())
            {
                if (entry.isManual && ! entry.customName.empty())
                {
                    std::string ident = sanitizeIdentifier(entry.customName);
                    clipsPrelude += "    inline const WaviateAudio " + ident + " = loadAudio(\"";
                    for (char c : entry.location)
                    {
                        if (c == '\\')
                            clipsPrelude += "/";
                        else
                            clipsPrelude += c;
                    }
                    clipsPrelude += "\");\n";
                }
            }
            clipsPrelude += "} // namespace Clips\n";
            tu.append(clipsPrelude);
        }

        tu.append("\n#line 1 \"shader.wlsl\"\n");
        tu.append(userSource);
        tu.append("\n\n#line 1 \"WaviateCppAbiShim.hpp\"\n");
        tu.append(buildCppAbiShim(userSource));
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
    std::vector<std::string> implicitIncludes;
    const llvm::Triple targetTriple(inv->getTargetOpts().Triple);
    clang::LangOptions::setLangDefaults(
        lang,
        cppMode ? clang::Language::CXX : clang::Language::C,
        targetTriple,
        implicitIncludes,
        cppMode ? clang::LangStandard::lang_cxx23 : clang::LangStandard::lang_c17);

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
    std::unique_ptr<llvm::MemoryBuffer> buffer,
    std::string& diagnostics
) {
    auto diagOpts = llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions>(new clang::DiagnosticOptions());
    llvm::raw_string_ostream diagStream(diagnostics);
    clang::TextDiagnosticPrinter diagClient(diagStream, diagOpts.get());

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
    if (!ci.ExecuteAction(action)) {
        diagStream.flush();
        return nullptr;
    }

    diagStream.flush();
    return action.takeModule();
}

template <bool cppMode>
std::unique_ptr<llvm::ExecutionEngine> ClangCompiler<cppMode>::buildJIT(std::unique_ptr<llvm::Module> m) {
    std::printf("[DEBUG] buildJIT start, module=%p\n", (void*)m.get());
    std::fflush(stdout);
    if (!m) return nullptr;

    for (auto& f : m->functions()) {
        std::printf("[DEBUG] JIT Function in Module: %s\n", f.getName().str().c_str());
        std::fflush(stdout);
    }

    llvm::Function* loadAudioFunc = m->getFunction("waviate_load_audio_from_location");
    llvm::Function* consumeFuelFunc = m->getFunction("waviate_consume_fuel");
    llvm::Function* fuelTrapFunc = m->getFunction("waviate_fuel_trap");
    llvm::Function* arenaAllocFunc = m->getFunction("__waviate_internal_arena_allocate");
    llvm::Function* arenaGenFunc = m->getFunction("__waviate_internal_arena_generation");

    std::printf("[DEBUG] buildJIT: calling registerRuntimeSymbols\n");
    std::fflush(stdout);
    waviate::safety::registerRuntimeSymbols();
    std::printf("[DEBUG] buildJIT: registerRuntimeSymbols finished\n");
    std::fflush(stdout);

    std::printf("[DEBUG] buildJIT: creating EngineBuilder\n");
    std::fflush(stdout);
    std::string err;
    llvm::ExecutionEngine* raw = llvm::EngineBuilder(std::move(m))
        .setEngineKind(llvm::EngineKind::JIT)
        .setErrorStr(&err)
        .create();

    std::printf("[DEBUG] buildJIT: EngineBuilder finished, raw=%p, err=%s\n", (void*)raw, err.c_str());
    std::fflush(stdout);
    if (!raw) return nullptr;

    if (loadAudioFunc) {
        std::printf("[DEBUG] buildJIT: mapping waviate_load_audio_from_location, func=%p\n", (void*)loadAudioFunc);
        std::fflush(stdout);
        raw->addGlobalMapping(loadAudioFunc, reinterpret_cast<void*>(&waviate::audio::waviate_load_audio_from_location));
    }
    if (consumeFuelFunc) {
        std::printf("[DEBUG] buildJIT: mapping waviate_consume_fuel, func=%p\n", (void*)consumeFuelFunc);
        std::fflush(stdout);
        raw->addGlobalMapping(consumeFuelFunc, reinterpret_cast<void*>(&waviate_consume_fuel));
    }
    if (fuelTrapFunc) {
        std::printf("[DEBUG] buildJIT: mapping waviate_fuel_trap, func=%p\n", (void*)fuelTrapFunc);
        std::fflush(stdout);
        raw->addGlobalMapping(fuelTrapFunc, reinterpret_cast<void*>(&waviate_fuel_trap));
    }
    if (arenaAllocFunc) {
        std::printf("[DEBUG] buildJIT: mapping __waviate_internal_arena_allocate, func=%p\n", (void*)arenaAllocFunc);
        std::fflush(stdout);
        raw->addGlobalMapping(arenaAllocFunc, reinterpret_cast<void*>(&__waviate_internal_arena_allocate));
    }
    if (arenaGenFunc) {
        std::printf("[DEBUG] buildJIT: mapping __waviate_internal_arena_generation, func=%p\n", (void*)arenaGenFunc);
        std::fflush(stdout);
        raw->addGlobalMapping(arenaGenFunc, reinterpret_cast<void*>(&__waviate_internal_arena_generation));
    }
    std::printf("[DEBUG] buildJIT: mapping finished\n");
    std::fflush(stdout);

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
    std::printf("[DEBUG] ClangCompiler::compileSource start\n");
    std::fflush(stdout);
    outSample = nullptr;
    outFrequency = nullptr;

    dispatch_.store(nullptr, std::memory_order_release);

    const char* virtualFilename = cppMode ? "shader.cpp" : "shader.c";

    if (const auto sourceDiagnostics = validateSourceCapabilities(source); !sourceDiagnostics.empty())
        throw std::runtime_error("Unsafe shader source rejected:\n" + sourceDiagnostics);

    std::printf("[DEBUG] ClangCompiler::compileSource: source validated\n");
    std::fflush(stdout);
    const std::string tu = buildTranslationUnit(source);
    std::printf("[DEBUG] ClangCompiler::compileSource: translation unit built\n");
    std::fflush(stdout);

    auto unit = std::make_unique<CompiledUnit>();
    unit->ctx = std::make_unique<llvm::LLVMContext>();

    auto buffer = llvm::MemoryBuffer::getMemBufferCopy(tu, virtualFilename);
    if (!buffer) return;

    std::string diagnostics;
    std::printf("[DEBUG] ClangCompiler::compileSource: calling emitLLVMModule\n");
    std::fflush(stdout);
    auto module = emitLLVMModule(*unit->ctx, std::move(buffer), diagnostics);
    std::printf("[DEBUG] ClangCompiler::compileSource: emitLLVMModule finished\n");
    std::fflush(stdout);
    if (!module) {
        if (!diagnostics.empty())
            throw std::runtime_error(diagnostics);

        throw std::runtime_error("Clang did not emit an LLVM module");
    }

    std::printf("[DEBUG] ClangCompiler::compileSource: calling validateModuleSafety pre-instrumentation\n");
    std::fflush(stdout);
    if (const auto validation = waviate::safety::validateModuleSafety(*module); !validation)
        throw std::runtime_error("Unsafe shader rejected before instrumentation:\n" + validation.diagnostics);

    std::printf("[DEBUG] ClangCompiler::compileSource: calling instrumentModuleWithFuel\n");
    std::fflush(stdout);
    waviate::safety::instrumentModuleWithFuel(*module);
    std::printf("[DEBUG] ClangCompiler::compileSource: instrumentModuleWithFuel finished\n");
    std::fflush(stdout);

    std::printf("[DEBUG] ClangCompiler::compileSource: calling validateModuleSafety post-instrumentation\n");
    std::fflush(stdout);
    if (const auto validation = waviate::safety::validateModuleSafety(*module); !validation)
        throw std::runtime_error("Unsafe shader rejected after instrumentation:\n" + validation.diagnostics);

    std::printf("[DEBUG] ClangCompiler::compileSource: calling buildJIT\n");
    std::fflush(stdout);
    unit->ee = buildJIT(std::move(module));
    std::printf("[DEBUG] ClangCompiler::compileSource: buildJIT finished, ee=%p\n", (void*)unit->ee.get());
    std::fflush(stdout);
    if (!unit->ee)
        throw std::runtime_error("Failed to create the JIT execution engine");

    std::printf("[DEBUG] ClangCompiler::compileSource: calling finalizeObject\n");
    std::fflush(stdout);
    unit->ee->finalizeObject();
    std::printf("[DEBUG] ClangCompiler::compileSource: finalizeObject finished\n");
    std::fflush(stdout);

    const uint64_t sampleAddr = unit->ee->getFunctionAddress("sample_process");
    std::printf("[DEBUG] ClangCompiler::compileSource: sample_process address found: %llu\n", (unsigned long long)sampleAddr);
    std::fflush(stdout);
    const uint64_t freqAddr = unit->ee->getFunctionAddress("frequency_process");
    std::printf("[DEBUG] ClangCompiler::compileSource: frequency_process address found: %llu\n", (unsigned long long)freqAddr);
    std::fflush(stdout);

    unit->dispatch.sample = sampleAddr ? reinterpret_cast<SampleShader>(sampleAddr) : nullptr;
    unit->dispatch.freq = freqAddr ? reinterpret_cast<FrequencyShader>(freqAddr) : nullptr;

    outSample = unit->dispatch.sample;
    outFrequency = unit->dispatch.freq;

    retireOldActive();
    active_ = std::move(unit);

    dispatch_.store(&active_->dispatch, std::memory_order_release);
}
