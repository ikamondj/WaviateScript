/*
  ==============================================================================

    CodeEditor.h - JUCE-based syntax-highlighted Waviate Shading Language
    editor with lazy initialization and integrated compilation.
    
    Created: 25 Feb 2026 2:00:23am
    Author:  ikamo

  ==============================================================================
*/

#pragma once

#include <functional>

#include <JuceHeader.h>
#include "AppTheme.h"
#include "CodeEditorCompletion.h"

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
                   private juce::KeyListener,
                   private juce::CodeDocument::Listener
{
public:
    explicit CodeEditor(WaviateScriptAudioProcessor* processor = nullptr);
    ~CodeEditor() override;

    /**
     * Set the processor reference for compilation functionality.
     */
    void setProcessor(WaviateScriptAudioProcessor& processor);

    /**
     * Attach the shared audio visualizer so it can live above the log panel.
     */
    void setVisualizer(juce::AudioVisualiserComponent& visualizerComponent);

    /**
     * Called after user edits to the document.
     */
    void setOnTextChanged(std::function<void()> callback);

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

    /**
     * Enable or disable editor autocomplete behavior.
     */
    void setCompletionsEnabled(bool shouldBeEnabled);
    bool areCompletionsEnabled() const { return areCompletionsEnabledFlag; }

    /**
     * Set the active source extension used for compiler and language tooling.
     */
    void setFileExtension(const juce::String& extension);

private:
    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key, juce::Component*) override;
    void codeDocumentTextInserted(const juce::String& newText, int insertIndex) override;
    void codeDocumentTextDeleted(int startIndex, int endIndex) override;

    // UI Layout helpers
    void layoutEditorArea();
    void layoutStatusBar();
    int getVisualizerHeight(int availableHeight) const;
    juce::Font createCodeEditorFont() const;
    void applyTheme();
    void updateVisualizerButton();
    void updatePlayPauseButton();
    void applyVisualizerScaleIndex(int index);
    void nudgeVisualizerScale(int direction);
    int getVisualizerSamplesPerBlock() const;
    void updateVisualizerScaleLabel();
    
    // Compilation
    void performCompilation();

    // Autocomplete helpers
    void updateCompletions();
    void handleCompletionAccepted(const CompletionItem& item);
    void triggerAutocompletionIfApplicable(juce::juce_wchar createdChar);

    // ===== References =====
    WaviateScriptAudioProcessor* audioProcessor = nullptr;
    std::function<void()> onTextChanged;

    class VisualizerWheelOverlay final : public juce::Component
    {
    public:
        std::function<void(const juce::MouseWheelDetails&)> onWheel;

    private:
        void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
        {
            if (onWheel != nullptr)
                onWheel(wheel);
        }
    };

    // ===== Editor Components =====
    juce::CodeDocument document;
    juce::CPlusPlusCodeTokeniser tokeniser;
    std::unique_ptr<juce::CodeEditorComponent> editor;

    // ===== Autocomplete =====
    std::unique_ptr<CompletionProvider> completionProvider;
    std::unique_ptr<CompletionPopupMenu> completionMenu;

    // ===== Bottom Status Bar =====
    juce::Component statusBar;
    juce::TextButton expandLogButton{ "Show Logs" };
    juce::TextButton expandVisualizerButton{ "Close Viz" };
    juce::TextButton playPauseButton{ "||" };
    juce::TextButton compileButton{ "Compile (Ctrl+Enter)" };
    
    // ===== Log Area (Expandable) =====
    juce::TextEditor logListBox;
    std::vector<juce::String> logMessages;
    juce::AudioVisualiserComponent* visualizer = nullptr;
    juce::Slider visualizerScaleSlider;
    juce::Label visualizerScaleValueLabel;
    VisualizerWheelOverlay visualizerWheelOverlay;
    
    // ===== State =====
    juce::String compilerExtension = ".wlsl";
    WaviateTheme activeTheme = WaviateThemes::fallback();
    bool isLogExpanded = false;
    bool isVisualizerExpanded = true;
    bool areCompletionsEnabledFlag = true;
    int visualizerSamplesPerBlockIndex = 3;
    static constexpr float codeFontHeight = 14.0f;
    static constexpr int collapsedStatusBarHeight = 28;
    static constexpr int expandedLogHeight = 120;
    static constexpr int minVisualizerHeight = 120;
    static constexpr int maxVisualizerHeight = 220;
    static constexpr int visualizerScaleControlWidth = 48;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CodeEditor)
};
