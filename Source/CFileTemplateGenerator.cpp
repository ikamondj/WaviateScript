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
    return "float sample_process(const WaviateSampleInput* input, WaviateSampleStateWriter* writer) {\n"
        "\treturn 0.0f;\n"
        "}\n"
        "\n"
        "/* Uncomment frequency_process below to enable frequency domain processing */\n"
        "//float frequency_process(const WaviateFrequencyInput* input, WaviateFrequencyStateWriter* writer) {\n"
        "//\tint channel = input->channel;\n"
        "//\tint bin = input->bin;\n"
        "//\treturn input->currentFrequencyData[channel][bin];\n"
        "//}\n";
}
