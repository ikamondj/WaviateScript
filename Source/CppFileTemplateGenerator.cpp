/*
  ==============================================================================

    CppFileTemplateGenerator.cpp
    Created: 27 Feb 2026 1:47:53am
    Author:  ikamo

  ==============================================================================
*/

#include "CppFileTemplateGenerator.h"

std::string CppFileTemplateGenerator::getDefaultFileSource() const
{
    return R"(float SampleProcess(const WaviateSample& wav) {
    return 0.0f;
}

/* Uncomment FrequencyProcess below to enable frequency domain processing */
// WaviateComplex FrequencyProcess(const WaviateFrequency& wav) {
//     int channel = wav.getChannel();
//     int bin = wav.getBin();
//     return wav.getIncomingSample(channel, bin);
// }
)";
}
