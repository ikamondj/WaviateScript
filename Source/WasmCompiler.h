/*
  ==============================================================================

    WasmCompiler.h
    Created: 18 Dec 2025 4:14:55pm
    Author:  ikamo

  ==============================================================================
*/

#pragma once
#include "AbstractCompiler.h"
class WasmCompiler : public AbstractCompiler {
    enum class SourceType : uint32_t {
        C,
        Cpp,
        Rust
    };
    void compileC(std::string source, SampleShader& outSample, FrequencyShader& outFrequency);
    void compileCpp(std::string source, SampleShader& outSample, FrequencyShader& outFrequency);
    void compileRust(std::string source, SampleShader& outSample, FrequencyShader& outFrequency);
public:
    SourceType sourceType;
    void compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency) override;
};