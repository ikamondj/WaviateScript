#pragma once
#include "AbstractCompiler.h"
#include <JuceHeader.h>
#include <memory>
class ClangExternalCompiler : public AbstractCompiler {
public:
    void compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency) override;
private:
    std::unique_ptr<juce::DynamicLibrary> loadedLibrary;
};