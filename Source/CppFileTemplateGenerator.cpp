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
    return R"(float SampleProcess(WaviateSample& wav) {
    return 0.0f;
}

/* Uncomment FrequencyProcess below to enable frequency domain processing */
// WaviateComplex FrequencyProcess(WaviateFrequency& wav) {
//     int channel = wav.channel();
//     int bin = wav.bin();
//     return wav.incomingSample(channel, bin);
// }
)";
}
