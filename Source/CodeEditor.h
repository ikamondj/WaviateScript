/*
  ==============================================================================

    CodeEditor.h
    Created: 25 Feb 2026 2:00:23am
    Author:  ikamo

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class CodeEditor : public juce::Component
{
public:
    CodeEditor();
    ~CodeEditor() override;

    void setText(const juce::String& text);
    juce::String getText() const;
    void setFileExtension(const juce::String& ext);
    void ensureEditorCreated();

private:
    void resized() override;

    juce::CodeDocument document;
    juce::CPlusPlusCodeTokeniser tokeniser;
    std::unique_ptr<juce::CodeEditorComponent> editor;
};