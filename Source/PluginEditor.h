/*
  ==============================================================================

    Professional JUCE plugin editor with toolbar, file menu, and keyboard shortcuts.

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
/**
 * Editor with professional toolbar, file menu system, and keyboard shortcuts.
 * Supports creating new files in multiple languages, opening/saving files,
 * with full keyboard support (Ctrl+S for Save, Ctrl+Shift+S for Save As).
 */
class WaviateScriptAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::KeyListener
{
public:
    explicit WaviateScriptAudioProcessorEditor(WaviateScriptAudioProcessor&);
    ~WaviateScriptAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key, juce::Component*) override;

private:
    // File menu operations
    void showFileMenu();
    void showNewFileMenu();
    void createNewFile(const juce::String& languageExtension);
    void openFile();
    void saveFile();
    void saveFileAs();
    
    // File dialog handling
    void handleNewFileDialogResult(const juce::FileChooser& chooser, 
                                   const juce::String& languageExtension,
                                   const juce::String& templateContent);
    void handleOpenFileDialogResult(const juce::FileChooser& chooser);
    void handleSaveFileDialogResult(const juce::FileChooser& chooser, 
                                    bool isNewFile);

    // File operations
    void loadScriptFile(const juce::File& file);
    void updateFileLabel();
    void showEmptyState();
    void hideEmptyState();
    
    // Template generation helpers
    juce::String getTemplateForLanguage(const juce::String& languageExtension) const;
    juce::String getLanguageDisplayName(const juce::String& languageExtension) const;
    juce::File getDefaultSaveDirectory() const;

    WaviateScriptAudioProcessor& audioProcessor;

    // ===== UI Components =====
    // Toolbar
    juce::Component toolbar;
    juce::TextButton fileMenuButton{ "File" };
    juce::TextButton helpMenuButton{ "Help" };
    
    // File info display
    juce::Label currentFileLabel;
    
    // Code editor - initialized with processor
    CodeEditor codeEditor;
    
    // Empty state UI
    juce::Label emptyStateLabel;
    juce::AudioVisualiserComponent& visualizer;

    // ===== File State =====
    juce::File currentScriptFile;
    std::unique_ptr<juce::FileChooser> fileChooser;
    
    // File state tracking
    bool isFileTransient = false;      // true = user hasn't saved the file yet
    bool isFileModified = false;       // true = file has unsaved changes

    // ===== Layout Constants =====
    static constexpr int toolbarHeight = 32;
    static constexpr int buttonWidth = 60;
    static constexpr int buttonHeight = 24;
    static constexpr int padding = 6;

    CfileTemplateGenerator cTemplateGen;
    CppFileTemplateGenerator cppTemplateGen;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaviateScriptAudioProcessorEditor)
};