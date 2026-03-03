/*
  ==============================================================================

    CFileTemplateGenerator.cpp
    Created: 27 Feb 2026 1:47:46am
    Author:  ikamo

  ==============================================================================
*/

#include "CFileTemplateGenerator.h"

std::string CfileTemplateGenerator::getDefaultFileSource()
{
    return R"(float sample_process(const WaviateSampleInput* input, WaviateSampleStateWriter* writer) {
    return 0.0f;
}

/* Uncomment frequency_process below to enable frequency domain processing */
// float frequency_process(const WaviateFrequencyInput* input, WaviateFrequencyStateWriter* writer) {
//     int channel = input->channel;
//     int bin = input->bin;
//     return input->currentFrequencyData[channel][bin];
// }
)";
}
