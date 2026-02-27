#pragma once

#include <JuceHeader.h>
#include <memory>
#include "AbstractCompiler.h"
;
class ClangExternalCompiler : public AbstractCompiler {
public:
    void compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency) override;
private:
    std::unique_ptr<juce::DynamicLibrary> loadedLibrary;
};