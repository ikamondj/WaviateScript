/*
  ==============================================================================

    Professional JUCE plugin editor with toolbar, file menu, and keyboard shortcuts.

  ==============================================================================
*/

#pragma once

#include <functional>

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "AppTheme.h"
#include "CodeEditor.h"
#include "CppFileTemplateGenerator.h"
#include "AudioClipsPanel.h"
#include "MarketplaceClient.h"

//==============================================================================
/**
 * Editor with professional toolbar, file menu system, and keyboard shortcuts.
 * Supports creating, opening, and saving Waviate Shading Language files with
 * full keyboard support (Ctrl+S for Save, Ctrl+Shift+S for Save As).
 */
class WaviateScriptAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::KeyListener,
                                         private juce::Timer
{
public:
    explicit WaviateScriptAudioProcessorEditor(WaviateScriptAudioProcessor&);
    ~WaviateScriptAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key, juce::Component*) override;
    void userTriedToCloseWindow() override;

private:
    // File menu operations
    void showFileMenu();
    void showViewMenu();
    void showToolsMenu();
    void showLoginMenu();
    void createNewFile();
    void openFile();
    void saveFile();
    void saveFileAs();
    void saveFileThen(std::function<void()> afterSave);
    void saveFileAsThen(std::function<void()> afterSave);
    
    // File dialog handling
    void handleNewFileDialogResult(const juce::FileChooser& chooser, 
                                   const juce::String& templateContent);
    void handleOpenFileDialogResult(const juce::FileChooser& chooser);
    void handleSaveFileDialogResult(const juce::FileChooser& chooser,
                                    std::function<void()> afterSave);

    // File operations
    void loadScriptFile(const juce::File& file);
    void openRecentFile(int index);
    void loadLastOpenedFileIfAvailable();
    void persistLastOpenedFile(const juce::File& file);
    void clearMissingLastOpenedFile();
    juce::StringArray getRecentFiles();
    void storeRecentFiles(const juce::StringArray& files);
    void rememberRecentFile(const juce::File& file);
    void markFileModified();
    void runAfterSavePrompt(std::function<void()> action);
    void requestApplicationQuit();
    void updateFileLabel();
    void showEmptyState();
    void hideEmptyState();
    
    // Template generation helpers
    juce::String getDefaultShaderTemplate() const;
    juce::File getDefaultSaveDirectory() const;
    void applyTheme(const WaviateTheme& theme, bool persistSelection);
    void selectTheme(const juce::String& themeId, bool persistSelection);
    void setCompletionsEnabled(bool shouldBeEnabled, bool persistSelection);
    void setFuelLimitPreset(waviate::compile::FuelLimitPreset preset, bool persistSelection);
    void beginMarketplaceLogin();
    void showMarketplaceTokenDialog();
    void handleMarketplaceSessionPaste(const juce::String& pastedSession);
    void uploadCurrentScriptToMarketplace();
    void updateMarketplaceButtons();
    static juce::PropertiesFile::Options createSettingsOptions();

    WaviateScriptAudioProcessor& audioProcessor;

    // ===== UI Components =====
    // Toolbar
    juce::Component toolbar;
    juce::TextButton fileMenuButton{ "File" };
    juce::TextButton viewMenuButton{ "View" };
    juce::TextButton toolsMenuButton{ "Tools" };
    juce::TextButton helpMenuButton{ "Help" };
    juce::TextButton loginButton{ "Login" };
    juce::TextButton uploadButton{ "Upload" };
    juce::LookAndFeel_V4 themedLookAndFeel;
    
    // File info display
    juce::Label currentFileLabel;
    
    // Code editor - initialized with processor
    CodeEditor codeEditor;
    
    // Empty state UI
    juce::Label emptyStateLabel;

    // ===== File State =====
    juce::File currentScriptFile;
    std::unique_ptr<juce::FileChooser> fileChooser;
    
    // File state tracking
    bool isFileTransient = false;      // true = user hasn't saved the file yet
    bool isFileModified = false;       // true = file has unsaved changes
    juce::String currentThemeId = WaviateThemes::fallback().id;
    std::unique_ptr<juce::PropertiesFile> userSettings;
    std::unique_ptr<MarketplaceClient> marketplaceClient;

    bool isAudioClipsPanelOpen = false;
    AudioClipsPanel audioClipsPanel;

    // ===== Layout Constants =====
    static constexpr int toolbarHeight = 32;
    static constexpr int buttonWidth = 60;
    static constexpr int accountButtonWidth = 78;
    static constexpr int uploadButtonWidth = 72;
    static constexpr int buttonHeight = 24;
    static constexpr int padding = 6;

    CppFileTemplateGenerator cppTemplateGen;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaviateScriptAudioProcessorEditor)
};
