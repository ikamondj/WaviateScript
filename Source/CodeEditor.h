/*
  ==============================================================================

    CodeEditor.h - JUCE-based syntax-highlighted code editor component with
    support for C, C++, and Rust (premium) with lazy initialization,
    language selection, and integrated compilation.
    
    Created: 25 Feb 2026 2:00:23am
    Author:  ikamo

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// Forward declaration
class WaviateScriptAudioProcessor;

/**
 * Professional code editor component with language selection, status bar, and compilation.
 * 
 * Features:
 * - Lazy initialization (editor created on first use)
 * - Language selector dropdown (C, C++, Rust with premium gating)
 * - Status bar with compile button and expandable log area
 * - Syntax highlighting with line numbers
 * - Tab-to-spaces conversion
 * - Integrated compilation with keyboard shortcut (Ctrl+Enter)
 * - Atomic result delivery to audio processor
 * 
 * Usage:
 *   CodeEditor editor(audioProcessor);
 *   addAndMakeVisible(editor);
 *   editor.setText("float process(float x) { return x * 2; }");
 *   editor.setLanguage(".wcpp");
 */
class CodeEditor : public juce::Component,
                   private juce::KeyListener
{
public:
    explicit CodeEditor(WaviateScriptAudioProcessor* processor = nullptr);
    ~CodeEditor() override;

    /**
     * Set the processor reference for compilation functionality.
     */
    void setProcessor(WaviateScriptAudioProcessor& processor);

    /**
     * Set the text content of the editor.
     * Auto-creates the editor component if not yet initialized.
     */
    void setText(const juce::String& text);

    /**
     * Get the current text content from the editor.
     */
    juce::String getText() const;

    /**
     * Set the language/file extension for syntax highlighting and compilation.
     * Supported: ".wc" (C), ".wcpp" (C++), ".wrs" (Rust - premium only)
     */
    void setLanguage(const juce::String& languageExtension);

    /**
     * Get the currently selected language extension.
     */
    juce::String getLanguage() const;

    /**
     * Explicitly ensure editor is created.
     * Called automatically by setText() and resized().
     */
    void ensureEditorCreated();
    
    /**
     * Compile the current editor content and send results to processor.
     * Results are stored in processor's atomic function pointers.
     */
    void compileCurrentSource();
    
    /**
     * Add a message to the compile log view.
     */
    void addLogMessage(const juce::String& message);
    
    /**
     * Clear all messages from the compile log.
     */
    void clearLog();

private:
    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key, juce::Component*) override;

    // Language selection
    void updateLanguageSelector();
    void onLanguageSelected(int languageIndex);
    
    // UI Layout helpers
    void layoutEditorArea();
    void layoutStatusBar();
    
    // Compilation
    void performCompilation();

    // ===== References =====
    WaviateScriptAudioProcessor* audioProcessor = nullptr;

    // ===== Editor Components =====
    juce::CodeDocument document;
    juce::CPlusPlusCodeTokeniser tokeniser;
    std::unique_ptr<juce::CodeEditorComponent> editor;

    // ===== Top Right: Language Selector =====
    std::unique_ptr<juce::ComboBox> languageSelector;
    
    // ===== Bottom Status Bar =====
    juce::Component statusBar;
    juce::TextButton expandLogButton{ "▼ Logs" };
    juce::TextButton compileButton{ "Compile (Ctrl+Enter)" };
    
    // ===== Log Area (Expandable) =====
    juce::TextEditor logListBox;
    std::vector<juce::String> logMessages;
    
    // ===== State =====
    juce::String currentLanguage = ".wcpp";  // Default to C++
    bool isLogExpanded = false;
    static constexpr int collapsedStatusBarHeight = 28;
    static constexpr int expandedLogHeight = 120;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CodeEditor)
};