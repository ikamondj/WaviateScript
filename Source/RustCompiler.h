#pragma once

#include "AbstractCompiler.h"

class RustCompiler : public AbstractCompiler {
public:
    void compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency) override;
};