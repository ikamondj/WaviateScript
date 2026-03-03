/*
  ==============================================================================

    CodeEditor.cpp
    Created: 25 Feb 2026 2:00:23am
    Author:  ikamo

  ==============================================================================
*/

#include "CodeEditor.h"

CodeEditor::CodeEditor() : juce::Component()
{
}

CodeEditor::~CodeEditor() = default;

void CodeEditor::ensureEditorCreated()
{
    if (editor == nullptr)
    {
        editor = std::make_unique<juce::CodeEditorComponent>(document, &tokeniser);
        
        editor->setTabSize(4, true);
        editor->setFont(juce::Font(14.0f));
        editor->setLineNumbersShown(true);
        editor->setColour(juce::CodeEditorComponent::backgroundColourId, juce::Colours::black.withAlpha(0.98f));
        editor->setColour(juce::CodeEditorComponent::defaultTextColourId, juce::Colours::whitesmoke);
        editor->setColour(juce::CaretComponent::caretColourId, juce::Colours::whitesmoke);
        editor->setColour(juce::CodeEditorComponent::highlightColourId, juce::Colours::cornflowerblue.withAlpha(0.25f));
        
        addAndMakeVisible(*editor);
        editor->setBounds(getLocalBounds());
    }
}

void CodeEditor::setText(const juce::String& text)
{
    document.replaceAllContent(text);
}

juce::String CodeEditor::getText() const
{
    return document.getAllContent();
}

void CodeEditor::setFileExtension(const juce::String& ext)
{
    // For now, we use CPlusPlusCodeTokeniser for all file types (C, C++, etc.)
    // Future enhancement: implement custom tokenizers for other languages
    (void)ext;  // Suppress unused parameter warning
}

void CodeEditor::resized()
{
    if (editor != nullptr)
    {
        editor->setBounds(getLocalBounds());
    }
}
