/*
  ==============================================================================

    CodeEditorCompletion.cpp - Completion provider implementation

  ==============================================================================
*/

#include "CodeEditorCompletion.h"
#include <algorithm>

//==============================================================================
// CompletionProvider
//==============================================================================

CompletionProvider::CompletionProvider() {
    buildWaviateCoreMembers();
    buildWaviateSampleMembers();
    buildWaviateFrequencyMembers();
    buildStructFields();
    buildGlobalSymbols();
}

std::vector<CompletionItem> CompletionProvider::getCompletions(const juce::String& sourceCode, int caretPos) {
    juce::String memberOwner;
    const auto prefix = extractCompletionContext(sourceCode, caretPos, memberOwner);

    // If we have a member owner, show member completions
    if (!memberOwner.isEmpty()) {
        auto completions = getMemberCompletions(memberOwner);
        return filterByPrefix(completions, prefix);
    }

    // Otherwise show global completions filtered by prefix
    return getGlobalCompletions(prefix);
}

std::vector<CompletionItem> CompletionProvider::getMemberCompletions(const juce::String& memberOwnerType) {
    std::vector<CompletionItem> result;

    if (memberOwnerType == "wav" || memberOwnerType == "WaviateSample" || memberOwnerType == "WaviateFrequency" || memberOwnerType == "WaviateCore") {
        result.insert(result.end(), waviateCoreMembers.begin(), waviateCoreMembers.end());
    }

    if (memberOwnerType == "wav" || memberOwnerType == "WaviateSample") {
        result.insert(result.end(), waviateSampleMembers.begin(), waviateSampleMembers.end());
    }

    if (memberOwnerType == "wav" || memberOwnerType == "WaviateFrequency") {
        result.insert(result.end(), waviateFrequencyMembers.begin(), waviateFrequencyMembers.end());
    }

    if (memberOwnerType == "WaviateSampleInput" || memberOwnerType == "input") {
        result.insert(result.end(), waviateSampleInputFields.begin(), waviateSampleInputFields.end());
    }

    if (memberOwnerType == "WaviateFrequencyInput" || memberOwnerType == "input") {
        result.insert(result.end(), waviateFrequencyInputFields.begin(), waviateFrequencyInputFields.end());
    }

    if (memberOwnerType == "WaviateComplex") {
        result.insert(result.end(), waviateComplexFields.begin(), waviateComplexFields.end());
    }

    return result;
}

std::vector<CompletionItem> CompletionProvider::getGlobalCompletions(const juce::String& prefix) {
    auto result = globalFunctions;
    auto keywords = globalKeywords;
    auto types = globalTypes;

    result.insert(result.end(), keywords.begin(), keywords.end());
    result.insert(result.end(), types.begin(), types.end());

    return filterByPrefix(sortByRelevance(result, prefix), prefix);
}

juce::String CompletionProvider::extractCompletionContext(const juce::String& sourceCode, int caretPos, 
                                                           juce::String& outMemberOwner) {
    outMemberOwner.clear();

    if (sourceCode.isEmpty())
        return {};

    caretPos = juce::jlimit(0, sourceCode.length(), caretPos);

    int prefixStart = caretPos;
    while (prefixStart > 0 && isIdentifierChar(sourceCode[prefixStart - 1]))
        --prefixStart;

    const auto prefix = sourceCode.substring(prefixStart, caretPos);

    int operatorEnd = prefixStart - 1;
    while (operatorEnd >= 0 && juce::CharacterFunctions::isWhitespace(sourceCode[operatorEnd]))
        --operatorEnd;

    const bool isDotAccess = operatorEnd >= 0 && sourceCode[operatorEnd] == '.';
    const bool isArrowAccess = operatorEnd >= 1 && sourceCode[operatorEnd - 1] == '-' && sourceCode[operatorEnd] == '>';

    if (! isDotAccess && ! isArrowAccess)
        return prefix;

    int ownerEnd = isDotAccess ? operatorEnd - 1 : operatorEnd - 2;
    while (ownerEnd >= 0 && juce::CharacterFunctions::isWhitespace(sourceCode[ownerEnd]))
        --ownerEnd;

    if (ownerEnd < 0 || ! isIdentifierChar(sourceCode[ownerEnd]))
        return prefix;

    int ownerStart = ownerEnd;
    while (ownerStart > 0 && isIdentifierChar(sourceCode[ownerStart - 1]))
        --ownerStart;

    outMemberOwner = sourceCode.substring(ownerStart, ownerEnd + 1);
    return prefix;
}

void CompletionProvider::buildWaviateCoreMembers() {
    waviateCoreMembers = {
        { "getSeconds", "getSeconds() -> float", "()", 1, CompletionItem::Kind::Function, "Get time in seconds since app start" },
        { "samplesToSeconds", "samplesToSeconds(samples) -> float", "(${1:samples})", 10, CompletionItem::Kind::Function, "Convert sample count to seconds" },
        { "secondsToSamples", "secondsToSamples(seconds) -> uint64_t", "(${1:seconds})", 11, CompletionItem::Kind::Function, "Convert seconds to sample count" },
        { "sampleRateHz", "sampleRateHz() -> float", "()", 1, CompletionItem::Kind::Function, "Get sample rate in Hz" },
        { "sampleRateKHz", "sampleRateKHz() -> float", "()", 1, CompletionItem::Kind::Function, "Get sample rate in kHz" },
        { "phase", "phase(x) -> float", "(${1:x})", 8, CompletionItem::Kind::Function, "Get fractional phase [0, 1)" },
        { "sine", "sine(x) -> float", "(${1:x})", 8, CompletionItem::Kind::Function, "Sine oscillator" },
        { "saw", "saw(x) -> float", "(${1:x})", 8, CompletionItem::Kind::Function, "Sawtooth wave [-1, 1]" },
        { "square", "square(x) -> float", "(${1:x})", 8, CompletionItem::Kind::Function, "Square wave ±1" },
        { "pulse", "pulse(x, width=0.5) -> float", "(${1:x}, ${2:0.5})", 18, CompletionItem::Kind::Function, "Pulse wave with variable width" },
        { "triangle", "triangle(x) -> float", "(${1:x})", 8, CompletionItem::Kind::Function, "Triangle wave [-1, 1]" },
        { "semicircle", "semicircle(x) -> float", "(${1:x})", 8, CompletionItem::Kind::Function, "Semicircle wave" },
        { "sawTan", "sawTan(x) -> float", "(${1:x})", 8, CompletionItem::Kind::Function, "Sawtooth using tan" },
        { "triangleTan", "triangleTan(x) -> float", "(${1:x})", 8, CompletionItem::Kind::Function, "Triangle using tan" },
        { "strongSine", "strongSine(x) -> float", "(${1:x})", 8, CompletionItem::Kind::Function, "Strong sine with harmonics" },
        { "fractalSquare", "fractalSquare(x) -> float", "(${1:x})", 8, CompletionItem::Kind::Function, "Fractal square wave" },
        { "perlin", "perlin(x, min=0, max=1) -> float", "(${1:x})", 8, CompletionItem::Kind::Function, "Perlin noise 1D" },
        { "simplex", "simplex(x, min=0, max=1) -> float", "(${1:x})", 8, CompletionItem::Kind::Function, "Simplex noise 1D" },
        { "voronoi", "voronoi(x, min=0, max=1) -> float", "(${1:x})", 8, CompletionItem::Kind::Function, "Voronoi noise 1D" },
        { "turbulence", "turbulence(x, octaves=4, lacunarity=2, gain=0.5, min=0, max=1) -> float", "(${1:x})", 8, CompletionItem::Kind::Function, "Turbulence (fBm)" },
        { "ridgedMulti", "ridgedMulti(x, octaves=4, lacunarity=2, gain=0.5, min=0, max=1) -> float", "(${1:x})", 8, CompletionItem::Kind::Function, "Ridged multifractal noise" },
        { "adsr", "adsr(a, d, s, r, t) -> float", "(${1:attack}, ${2:decay}, ${3:sustain}, ${4:release}, ${5:t})", 45, CompletionItem::Kind::Function, "ADSR envelope" },
        { "ADSR", "ADSR(a, d, s, r, t) -> float", "(${1:attack}, ${2:decay}, ${3:sustain}, ${4:release}, ${5:t})", 45, CompletionItem::Kind::Function, "ADSR envelope (uppercase)" },
    };
}

void CompletionProvider::buildWaviateSampleMembers() {
    waviateSampleMembers = {
        { "getChannel", "getChannel() -> int", "()", 1, CompletionItem::Kind::Function, "Get current channel number" },
        { "getSampleInBlock", "getSampleInBlock() -> int", "()", 1, CompletionItem::Kind::Function, "Get sample index in block" },
        { "getBlockSize", "getBlockSize() -> int", "()", 1, CompletionItem::Kind::Function, "Get block size" },
        { "getInputChannelCount", "getInputChannelCount() -> int", "()", 1, CompletionItem::Kind::Function, "Get input channel count" },
        { "getSideChainChannelCount", "getSideChainChannelCount() -> int", "()", 1, CompletionItem::Kind::Function, "Get sidechain channel count" },
        { "getChannelCount", "getChannelCount() -> int", "()", 1, CompletionItem::Kind::Function, "Get total channel count" },
        { "getSampleRate", "getSampleRate() -> float", "()", 1, CompletionItem::Kind::Function, "Get sample rate" },
        { "getSamplesSinceAppStart", "getSamplesSinceAppStart() -> uint64_t", "()", 1, CompletionItem::Kind::Function, "Get samples since app start" },
        { "isSustainDown", "isSustainDown() -> bool", "()", 1, CompletionItem::Kind::Function, "Check if sustain pedal is down" },
        { "getIncomingSample", "getIncomingSample(channel, sample) -> float", "(${1:-1}, ${2:-1})", 17, CompletionItem::Kind::Function, "Get input sample" },
        { "getSideChainSample", "getSideChainSample(channel, sample) -> float", "(${1:0}, ${2:-1})", 15, CompletionItem::Kind::Function, "Get sidechain sample" },
        { "getCurrentSample", "getCurrentSample(channel, sample) -> float", "(${1:-1}, ${2:-1})", 17, CompletionItem::Kind::Function, "Get current output sample" },
        { "setCurrentSample", "setCurrentSample(value, channel, sample)", "(${1:value})", 14, CompletionItem::Kind::Function, "Set current output sample" },
        { "isMidiNoteOn", "isMidiNoteOn(note) -> bool", "(${1:note})", 12, CompletionItem::Kind::Function, "Check if MIDI note is playing" },
        { "getMidiCCValue", "getMidiCCValue(controller) -> uint8_t", "(${1:controller})", 18, CompletionItem::Kind::Function, "Get MIDI CC value" },
    };
}

void CompletionProvider::buildWaviateFrequencyMembers() {
    waviateFrequencyMembers = {
        { "getChannel", "getChannel() -> int", "()", 1, CompletionItem::Kind::Function, "Get current channel" },
        { "getBin", "getBin() -> int", "()", 1, CompletionItem::Kind::Function, "Get current frequency bin" },
        { "getTotalBinCount", "getTotalBinCount() -> int", "()", 1, CompletionItem::Kind::Function, "Get total bin count" },
        { "getSampleWidth", "getSampleWidth() -> int", "()", 1, CompletionItem::Kind::Function, "Get FFT window size" },
        { "getChannelCount", "getChannelCount() -> int", "()", 1, CompletionItem::Kind::Function, "Get channel count" },
        { "getSampleRate", "getSampleRate() -> float", "()", 1, CompletionItem::Kind::Function, "Get sample rate" },
        { "getSamplesSinceAppStart", "getSamplesSinceAppStart() -> uint64_t", "()", 1, CompletionItem::Kind::Function, "Get samples since app start" },
        { "getIncomingSample", "getIncomingSample(channel, bin) -> WaviateComplex", "(${1:-1}, ${2:-1})", 17, CompletionItem::Kind::Function, "Get input frequency sample" },
        { "getCurrentSample", "getCurrentSample(channel, bin) -> WaviateComplex", "(${1:-1}, ${2:-1})", 17, CompletionItem::Kind::Function, "Get current frequency output" },
        { "getSideChainSample", "getSideChainSample(channel, bin) -> WaviateComplex", "(${1:0}, ${2:-1})", 15, CompletionItem::Kind::Function, "Get sidechain frequency sample" },
    };
}

void CompletionProvider::buildStructFields() {
    // WaviateSampleInput struct
    waviateSampleInputFields = {
        { "samplesSinceAppStart", "samplesSinceAppStart", "", 0, CompletionItem::Kind::Field, "uint64_t: Samples since app start" },
        { "sampleInBlock", "sampleInBlock", "", 0, CompletionItem::Kind::Field, "int32_t: Sample index in block" },
        { "blockSize", "blockSize", "", 0, CompletionItem::Kind::Field, "int32_t: Block size" },
        { "inputChannelCount", "inputChannelCount", "", 0, CompletionItem::Kind::Field, "int32_t: Input channel count" },
        { "sideChainChannelCount", "sideChainChannelCount", "", 0, CompletionItem::Kind::Field, "int32_t: Sidechain channel count" },
        { "sampleMemoryCount", "sampleMemoryCount", "", 0, CompletionItem::Kind::Field, "int32_t: Sample memory count" },
        { "channelCount", "channelCount", "", 0, CompletionItem::Kind::Field, "int32_t: Total channel count" },
        { "channel", "channel", "", 0, CompletionItem::Kind::Field, "uint8_t: Current channel" },
        { "midiNoteOn", "midiNoteOn", "", 0, CompletionItem::Kind::Field, "uint8_t*: MIDI note on array" },
        { "midiCCValue", "midiCCValue", "", 0, CompletionItem::Kind::Field, "uint8_t*: MIDI CC values" },
        { "sustain", "sustain", "", 0, CompletionItem::Kind::Field, "bool: Sustain pedal state" },
        { "controllerCount", "controllerCount", "", 0, CompletionItem::Kind::Field, "int32_t: Game controller count" },
        { "controllerAxisValue", "controllerAxisValue", "", 0, CompletionItem::Kind::Field, "float*: Controller axis values" },
        { "sampleRate", "sampleRate", "", 0, CompletionItem::Kind::Field, "float: Sample rate in Hz" },
        { "previousSamples", "previousSamples", "", 0, CompletionItem::Kind::Field, "float**: Previous sample history" },
        { "inputDeviceSamples", "inputDeviceSamples", "", 0, CompletionItem::Kind::Field, "const float* const*: Input samples" },
        { "inputSideChainSamples", "inputSideChainSamples", "", 0, CompletionItem::Kind::Field, "const float* const*: Sidechain samples" },
        { "currentSampleData", "currentSampleData", "", 0, CompletionItem::Kind::Field, "float* const*: Output samples" },
    };

    // WaviateFrequencyInput struct
    waviateFrequencyInputFields = {
        { "sampleWidth", "sampleWidth", "", 0, CompletionItem::Kind::Field, "int32_t: FFT window size" },
        { "bin", "bin", "", 0, CompletionItem::Kind::Field, "int32_t: Frequency bin index" },
        { "totalBinCount", "totalBinCount", "", 0, CompletionItem::Kind::Field, "int32_t: Total bin count" },
        { "channelCount", "channelCount", "", 0, CompletionItem::Kind::Field, "int32_t: Channel count" },
        { "channel", "channel", "", 0, CompletionItem::Kind::Field, "uint8_t: Current channel" },
        { "currentFrequencyData", "currentFrequencyData", "", 0, CompletionItem::Kind::Field, "const WaviateComplex**: Current frequency data" },
        { "inputDeviceData", "inputDeviceData", "", 0, CompletionItem::Kind::Field, "const WaviateComplex**: Input frequency data" },
        { "inputSideChainFrequencyData", "inputSideChainFrequencyData", "", 0, CompletionItem::Kind::Field, "const WaviateComplex**: Sidechain frequency data" },
        { "sampleRate", "sampleRate", "", 0, CompletionItem::Kind::Field, "float: Sample rate in Hz" },
        { "samplesSinceAppStart", "samplesSinceAppStart", "", 0, CompletionItem::Kind::Field, "uint64_t: Samples since app start" },
    };

    // WaviateComplex struct
    waviateComplexFields = {
        { "real", "real", "", 0, CompletionItem::Kind::Field, "float: Real component" },
        { "imag", "imag", "", 0, CompletionItem::Kind::Field, "float: Imaginary component" },
    };
}

void CompletionProvider::buildGlobalSymbols() {
    globalTypes = {
        { "WaviateSample", "WaviateSample", "", 0, CompletionItem::Kind::Class, "Sample processing context" },
        { "WaviateFrequency", "WaviateFrequency", "", 0, CompletionItem::Kind::Class, "Frequency processing context" },
        { "WaviateCore", "WaviateCore", "", 0, CompletionItem::Kind::Class, "Base class with oscillators and noise" },
        { "WaviateSampleInput", "WaviateSampleInput", "", 0, CompletionItem::Kind::Class, "Input data for sample processing" },
        { "WaviateFrequencyInput", "WaviateFrequencyInput", "", 0, CompletionItem::Kind::Class, "Input data for frequency processing" },
        { "WaviateComplex", "WaviateComplex", "", 0, CompletionItem::Kind::Class, "Complex number (real + imaginary)" },
        { "float", "float", "", 0, CompletionItem::Kind::Type, "Floating point number" },
        { "int", "int", "", 0, CompletionItem::Kind::Type, "Integer" },
        { "bool", "bool", "", 0, CompletionItem::Kind::Type, "Boolean" },
        { "uint8_t", "uint8_t", "", 0, CompletionItem::Kind::Type, "Unsigned 8-bit integer" },
        { "uint64_t", "uint64_t", "", 0, CompletionItem::Kind::Type, "Unsigned 64-bit integer" },
    };

    globalKeywords = {
        { "if", "if", "", 0, CompletionItem::Kind::Keyword, "Conditional statement" },
        { "else", "else", "", 0, CompletionItem::Kind::Keyword, "Else clause" },
        { "for", "for", "", 0, CompletionItem::Kind::Keyword, "For loop" },
        { "while", "while", "", 0, CompletionItem::Kind::Keyword, "While loop" },
        { "return", "return", "", 0, CompletionItem::Kind::Keyword, "Return statement" },
        { "const", "const", "", 0, CompletionItem::Kind::Keyword, "Constant qualifier" },
        { "static", "static", "", 0, CompletionItem::Kind::Keyword, "Static qualifier" },
        { "struct", "struct", "", 0, CompletionItem::Kind::Keyword, "Struct definition" },
        { "class", "class", "", 0, CompletionItem::Kind::Keyword, "Class definition" },
        { "inline", "inline", "", 0, CompletionItem::Kind::Keyword, "Inline function" },
    };

    globalFunctions = {
        { "SampleProcess", "SampleProcess(wav: WaviateSample) -> float", "", 0, CompletionItem::Kind::Function, "Your main sample processing function" },
        { "FrequencyProcess", "FrequencyProcess(wav: WaviateFrequency) -> WaviateComplex", "", 0, CompletionItem::Kind::Function, "Your main frequency processing function" },
    };
}

bool CompletionProvider::isIdentifierChar(juce::juce_wchar c) {
    return juce::CharacterFunctions::isLetterOrDigit(c) || c == '_';
}

std::vector<CompletionItem> CompletionProvider::filterByPrefix(const std::vector<CompletionItem>& items,
                                                                 const juce::String& prefix) {
    if (prefix.isEmpty())
        return items;

    std::vector<CompletionItem> result;
    for (const auto& item : items) {
        if (item.name.startsWithIgnoreCase(prefix)) {
            result.push_back(item);
        }
    }
    return result;
}

std::vector<CompletionItem> CompletionProvider::sortByRelevance(const std::vector<CompletionItem>& items,
                                                                  const juce::String& prefix) {
    auto result = items;
    
    std::sort(result.begin(), result.end(), [&prefix](const CompletionItem& a, const CompletionItem& b) {
        // Exact match first
        if (a.name == prefix && b.name != prefix) return true;
        if (b.name == prefix && a.name != prefix) return false;

        // Prefix match length (shorter is better)
        int aLen = a.name.length();
        int bLen = b.name.length();
        if (aLen != bLen) return aLen < bLen;

        // Alphabetical as tiebreaker
        return a.name < b.name;
    });

    return result;
}

//==============================================================================
// CompletionPopupMenu
//==============================================================================

CompletionPopupMenu::CompletionPopupMenu(juce::CodeEditorComponent* editorComponent)
    : editor(editorComponent) {
    setInterceptsMouseClicks(true, true);
    addKeyListener(this);
}

CompletionPopupMenu::~CompletionPopupMenu() {
    removeKeyListener(this);
}

void CompletionPopupMenu::showCompletions(const std::vector<CompletionItem>& items) {
    completionItems = items;
    selectedIndex = 0;

    if (editor == nullptr || completionItems.empty()) {
        hideCompletions();
        return;
    }

    // Position menu at caret
    const auto caretBounds = editor->getCharacterBounds(editor->getCaretPos());
    const auto popupPos = getParentComponent() != nullptr
        ? getParentComponent()->getLocalPoint(editor, caretBounds.getBottomLeft())
        : caretBounds.getBottomLeft();
    const auto popupHeight = juce::jmin(static_cast<int>(completionItems.size()), maxVisibleItems) * itemHeight;
    auto popupBounds = juce::Rectangle<int>(popupPos.x, popupPos.y, menuWidth, popupHeight);

    if (auto* parent = getParentComponent())
        popupBounds = popupBounds.constrainedWithin(parent->getLocalBounds());

    setBounds(popupBounds);

    setVisible(true);
    toFront(false);
    repaint();
}

void CompletionPopupMenu::hideCompletions() {
    completionItems.clear();
    selectedIndex = 0;
    setVisible(false);
}

bool CompletionPopupMenu::acceptCompletion() {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(completionItems.size()))
        return false;

    if (onCompletionAccepted != nullptr) {
        onCompletionAccepted(completionItems[selectedIndex]);
    }

    return true;
}

void CompletionPopupMenu::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff2d2d30)); // Dark background

    for (size_t i = 0; i < completionItems.size() && i < static_cast<size_t>(maxVisibleItems); ++i) {
        auto rect = juce::Rectangle<int>(0, static_cast<int>(i) * itemHeight, menuWidth, itemHeight);

        if (static_cast<int>(i) == selectedIndex) {
            g.setColour(juce::Colour(0xff0e7fc0)); // Blue highlight
            g.fillRect(rect);
        }

        g.setColour(juce::Colours::white);
        g.setFont(13.0f);
        g.drawText(completionItems[i].displayText, rect.reduced(4, 2), juce::Justification::centredLeft, true);
    }

    g.setColour(juce::Colour(0xff403d3d));
    g.drawRect(getLocalBounds(), 1);
}

void CompletionPopupMenu::resized() {
}

bool CompletionPopupMenu::keyPressed(const juce::KeyPress& key, juce::Component*) {
    if (key.getKeyCode() == juce::KeyPress::upKey) {
        selectedIndex = juce::jmax(0, selectedIndex - 1);
        repaint();
        return true;
    }

    if (key.getKeyCode() == juce::KeyPress::downKey) {
        selectedIndex = juce::jmin(static_cast<int>(completionItems.size()) - 1, selectedIndex + 1);
        repaint();
        return true;
    }

    if (key.getKeyCode() == juce::KeyPress::returnKey || key.getKeyCode() == juce::KeyPress::tabKey) {
        return acceptCompletion();
    }

    if (key.getKeyCode() == juce::KeyPress::escapeKey) {
        hideCompletions();
        return true;
    }

    return false;
}

void CompletionPopupMenu::mouseUp(const juce::MouseEvent& e) {
    int itemIndex = e.y / itemHeight;
    if (itemIndex >= 0 && itemIndex < static_cast<int>(completionItems.size())) {
        selectedIndex = itemIndex;
        acceptCompletion();
    }
}

void CompletionPopupMenu::mouseMove(const juce::MouseEvent& e) {
    int itemIndex = e.y / itemHeight;
    if (itemIndex >= 0 && itemIndex < static_cast<int>(completionItems.size())) {
        selectedIndex = itemIndex;
        repaint();
    }
}
