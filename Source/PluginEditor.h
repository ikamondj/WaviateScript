/*
  ==============================================================================

    Simple JUCE plugin editor: top bar with Open button + current script name.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CFileTemplateGenerator.h"
#include "CppFileTemplateGenerator.h"
#include "RustFileTemplateGenerator.h"

//==============================================================================
class WaviateScriptAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit WaviateScriptAudioProcessorEditor(WaviateScriptAudioProcessor&);
    ~WaviateScriptAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void onNewCClicked();
    void onNewCppClicked();
    void onNewRustClicked();
    void onOpenClicked();
    void createNewFileWithTemplate(const juce::String& fileExtension, const juce::String& templateContent);
    void setLoadedScriptFile(const juce::File& file);

    WaviateScriptAudioProcessor& audioProcessor;

    // Top bar UI
    juce::Component topBar;
    juce::TextButton newCButton{ "New C" };
    juce::TextButton newCppButton{ "New C++" };
    juce::TextButton newRustButton{ "New Rust" };
    juce::TextButton openButton{ "Open…" };
    juce::Label currentScriptLabel;

    juce::File currentScriptFile;
    std::unique_ptr<juce::FileChooser> fileChooser;


    static constexpr int topBarHeight = 36;

    CfileTemplateGenerator cGen;
    CppFileTemplateGenerator cppGen;
    RustFileTemplateGenerator rustGen;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaviateScriptAudioProcessorEditor)
};