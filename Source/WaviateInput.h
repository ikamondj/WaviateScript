/*
  ==============================================================================

    WaviateInput.h
    Created: 18 Dec 2025 4:16:36pm
    Author:  ikamo

  ==============================================================================
*/
#include <cstdbool>

#pragma once
struct WaviateSampleInput {
    uint64_t samplesSinceAppStart;
    int32_t positionInBlock;
    int32_t blockSize;
    int32_t midiSegmentSize;
    int32_t inputChannelCount;
    int32_t sideChainChannelCount;
    int32_t sampleMemoryCount;
    int32_t outputChannelCount;
    uint8_t outputChannel;

    uint8_t* midiNoteOn;
    uint8_t* midiCCValue;
    uint64_t* sampleWhenMidiNoteOn;
    uint64_t* sampleWhenMidiNoteOff;
    uint64_t* sampleWhenCCValueChanged;

    int32_t controllerCount;
    uint64_t* controllerButtonMask;
    uint64_t* sampleWhenControllerButtonChanged;
    int32_t controllerButtonCount;
    float* controllerAxisValue;
    uint64_t* sampleWhenControllerAxisChanged;
    int32_t controllerAxisCount;

    float sampleRate;
    float** previousSamples;
    const float* const* inputDeviceSamples;
    const float* const* inputSideChainSamples;
    float* const* currentSampleData;
};