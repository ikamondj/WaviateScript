/*
  ==============================================================================

    WasmCompiler.cpp
    Created: 18 Dec 2025 4:14:55pm
    Author:  ikamo

  ==============================================================================
*/

#include "WasmCompiler.h"

void WasmCompiler::compileC(std::string source, SampleShader& outSample, FrequencyShader& outFrequency)
{
    
}

void WasmCompiler::compileCpp(std::string source, SampleShader& outSample, FrequencyShader& outFrequency)
{

}

void WasmCompiler::compileRust(std::string source, SampleShader& outSample, FrequencyShader& outFrequency)
{

}

void WasmCompiler::compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency)
{
    switch (sourceType) {
    case SourceType::C:
        return compileC(source, outSample, outFrequency);
    case SourceType::Cpp:
        return compileCpp(source, outSample, outFrequency);
    case SourceType::Rust:
        return compileRust(source, outSample, outFrequency);
    }
}
