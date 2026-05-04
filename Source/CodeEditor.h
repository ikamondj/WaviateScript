/*
  ==============================================================================

    CodeEditor.h - JUCE-based syntax-highlighted Waviate Shading Language
    editor with lazy initialization and integrated compilation.
    
    Created: 25 Feb 2026 2:00:23am
    Author:  ikamo

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "AppTheme.h"

// Forward declaration
class WaviateScriptAudioProcessor;

/**
 * Professional code editor component with status bar and compilation.
 * 
 * Features:
 * - Lazy initialization (editor created on first use)
 * - Status bar with compile button and expandable log area
 * - Syntax highlighting with line numbers
 * - Tab-to-spaces conversion
 * - Integrated compilation with keyboard shortcut (Ctrl+Enter)
 * - Atomic result delivery to audio processor
 * 
 * Usage:
 *   CodeEditor editor(audioProcessor);
 *   addAndMakeVisible(editor);
 *   editor.setText("float SampleProcess(const WaviateSample& wav) { return 0.0f; }");
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

    /**
     * Apply an app theme to the editor chrome, syntax colours, and log surface.
     */
    void setTheme(const WaviateTheme& theme);

private:
    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key, juce::Component*) override;

    // UI Layout helpers
    void layoutEditorArea();
    void layoutStatusBar();
    juce::Font createCodeEditorFont() const;
    void applyTheme();
    
    // Compilation
    void performCompilation();

    // ===== References =====
    WaviateScriptAudioProcessor* audioProcessor = nullptr;

    // ===== Editor Components =====
    juce::CodeDocument document;
    juce::CPlusPlusCodeTokeniser tokeniser;
    std::unique_ptr<juce::CodeEditorComponent> editor;

    // ===== Bottom Status Bar =====
    juce::Component statusBar;
    juce::TextButton expandLogButton{ "Show Logs" };
    juce::TextButton compileButton{ "Compile (Ctrl+Enter)" };
    
    // ===== Log Area (Expandable) =====
    juce::TextEditor logListBox;
    std::vector<juce::String> logMessages;
    
    // ===== State =====
    juce::String compilerExtension = ".wsl";
    WaviateTheme activeTheme = WaviateThemes::fallback();
    bool isLogExpanded = false;
    static constexpr float codeFontHeight = 14.0f;
    static constexpr int collapsedStatusBarHeight = 28;
    static constexpr int expandedLogHeight = 120;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CodeEditor)
};
