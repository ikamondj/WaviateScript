#pragma once
#include "WaviateInput.h"

#include <string>

using SampleShader = float (*) (const WaviateSampleInput*, WaviateSampleStateWriter*);
using FrequencyShader = WaviateComplex (*) (const WaviateFrequencyInput*, WaviateFrequencyStateWriter*);

class AbstractCompiler {
public:
    virtual ~AbstractCompiler() = default;
    virtual void compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency) = 0;
};
