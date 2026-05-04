/*
  ==============================================================================

    WaviateInput.h
    Created: 18 Dec 2025 4:16:36pm
    Author:  ikamo

  ==============================================================================
*/
#include <cstdbool>
#include <cstdint>

#pragma once

#ifndef WAVIATE_SCRIPT_INPUT_API_DEFINED
#define WAVIATE_SCRIPT_INPUT_API_DEFINED

struct WaviateSampleInput {
    uint64_t samplesSinceAppStart;
    int32_t sampleInBlock;
    int32_t blockSize;
    int32_t inputChannelCount;
    int32_t sideChainChannelCount;
    int32_t sampleMemoryCount;
    int32_t channelCount;
    uint8_t channel;

    uint8_t* midiNoteOn;
    uint8_t* midiCCValue;
    uint64_t* sampleWhenMidiNoteOn;
    uint64_t* sampleWhenMidiNoteOff;
    uint64_t* sampleWhenCCValueChanged;
    bool sustain;
    bool* sustainDefer;

    int32_t controllerCount;
    uint64_t* controllerButtonMask;
    uint64_t* sampleWhenControllerButtonChanged;
    int32_t controllerButtonCount;
    float* controllerAxisValue;
    uint64_t* sampleWhenControllerAxisChanged;
    int32_t controllerAxisCount;

#ifdef WAV_SCRIPT_PREMIUM
	const char* const* oscStrings;
	const int32_t* oscInts;
	const uint32_t* oscColors;
    const float* oscFloats;
#endif

    float sampleRate;
    float** previousSamples;
    const float* const* inputDeviceSamples;
    const float* const* inputSideChainSamples;
    float* const* currentSampleData;
};

struct WaviateSampleStateWriter {

};

struct WaviateComplex {
    float real;
    float imag;
};

struct WaviateFrequencyInput {
    int32_t sampleWidth;
    int32_t bin;
    int32_t totalBinCount;
    int32_t channelCount;
    uint8_t channel;

    const WaviateComplex** currentFrequencyData;
    const WaviateComplex** inputDeviceData;
    const WaviateComplex** inputSideChainFrequencyData;

    float sampleRate;
    uint64_t samplesSinceAppStart;
};

struct WaviateFrequencyStateWriter {

};

#endif

