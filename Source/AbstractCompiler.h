#pragma once
#include <string>
typedef float (*SampleShader)(const class WaviateSampleInput*, class WaviateSampleStateWriter*);
typedef float (*FrequencyShader)(const class WaviateFrequencyInput*, class WaviateFrequencyStateWriter*);

class AbstractCompiler {

public:
    virtual void compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency);
};