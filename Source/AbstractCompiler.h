#pragma once
#include <string>

typedef float (*SampleShader)(const struct WaviateSampleInput*, struct WaviateSampleStateWriter*);
typedef struct WaviateComplex (*FrequencyShader)(const struct WaviateFrequencyInput*, struct WaviateFrequencyStateWriter*);

class AbstractCompiler {

public:
    virtual void compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency) {};
};