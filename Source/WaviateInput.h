/*
  ==============================================================================

    WaviateInput.h
    Created: 18 Dec 2025 4:16:36pm
    Author:  ikamo

  ==============================================================================
*/
#include <cstdbool>

#pragma once
struct WavInput {
    const float* const* inputSamples;
    int numSamples;
    const float* sideChainSamples;
    bool usesSideChain;
    int numSideChainSamples;
    int numInputChannels;
    int numOutputChannels;
    bool midiNote[128];
    float midiControllersCC[128];
    int currentOutputChannel;
    const float* const* outputSamples;
};