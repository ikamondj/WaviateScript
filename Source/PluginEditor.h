/*
  ==============================================================================

    Simple JUCE plugin editor: top bar with Open button + current script name.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CodeEditor.h"
#include "CFileTemplateGenerator.h"
#include "CppFileTemplateGenerator.h"
#ifdef WAV_SCRIPT_PREMIUM
#include "RustFileTemplateGenerator.h"
#endif

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
#ifdef WAV_SCRIPT_PREMIUM
    void onNewRustClicked();
#endif
    void onOpenClicked();
    void createNewFileWithTemplate(const juce::String& fileExtension, const juce::String& templateContent);
    void setLoadedScriptFile(const juce::File& file);
    void showEmptyState();
    void hideEmptyState();

    WaviateScriptAudioProcessor& audioProcessor;

    // Top bar UI
    juce::Component topBar;
    juce::TextButton newCButton{ "New C" };
    juce::TextButton newCppButton{ "New C++" };
#ifdef WAV_SCRIPT_PREMIUM
    juce::TextButton newRustButton{ "New Rust" };
#endif
    juce::TextButton openButton{ "Open…" };
    juce::Label currentScriptLabel;

    // Code editor
    CodeEditor codeEditor;
    
    // Empty state UI
    juce::Label emptyStateLabel;
    
    // File tracking
    juce::File currentScriptFile;
    std::unique_ptr<juce::FileChooser> fileChooser;
    bool isUnsavedNewFile = false;


    static constexpr int topBarHeight = 36;

    CfileTemplateGenerator cGen;
    CppFileTemplateGenerator cppGen;
#ifdef WAV_SCRIPT_PREMIUM
    RustFileTemplateGenerator rustGen;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaviateScriptAudioProcessorEditor)
};