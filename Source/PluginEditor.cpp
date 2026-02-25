/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_gui_extra/juce_gui_extra.h>

#if JUCE_WINDOWS
#include <windows.h>
#include <shlobj.h>
#endif

static juce::File getUiRoot()
{
#if JUCE_WINDOWS
    auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    // userApplicationDataDirectory is typically Roaming on Windows.
    // Prefer LocalAppData for assets:
    auto local = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile("AppData")
        .getChildFile("Local");

    return local.getChildFile("waviate")
        .getChildFile("script")
        .getChildFile("ui");
#elif JUCE_MAC
    // ~/Library/Application Support
    auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    return base.getChildFile("waviate")
        .getChildFile("script")
        .getChildFile("ui");
#else
    // Linux: pick something similar, e.g. ~/.local/share
    auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    return base.getChildFile("waviate")
        .getChildFile("script")
        .getChildFile("ui");
#endif
}
static juce::String getIndexUrl()
{
    auto uiRoot = getUiRoot();
    auto index = uiRoot.getChildFile("index.html");
    return index.getFullPathName();
}

//==============================================================================
WaviateScriptAudioProcessorEditor::WaviateScriptAudioProcessorEditor (WaviateScriptAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
    juce::WebBrowserComponent::Options opts;
    opts = opts.withBackend(juce::WebBrowserComponent::Options::Backend::webview2);
    
    codeDoc.insertText(0, juce::String("test"));
    
    codeEditor = std::make_unique<juce::CodeEditorComponent>(codeDoc, &tokenizer);
    addAndMakeVisible(codeEditor.get());
    //webBrowser->goToURL("http://localhost:3000");
}

WaviateScriptAudioProcessorEditor::~WaviateScriptAudioProcessorEditor()
{
}

//==============================================================================
void WaviateScriptAudioProcessorEditor::paint (juce::Graphics& g)
{
    
}

void WaviateScriptAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    //if (webBrowser) {
    //    webBrowser->setSize(getWidth(), getHeight());
    //}
    if (codeEditor) {
        codeEditor->setSize(getWidth(), getHeight());
    }
    
}
