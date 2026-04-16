/*
  ==============================================================================

    Professional JUCE plugin editor with toolbar, file menu, and keyboard shortcuts.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
WaviateScriptAudioProcessorEditor::WaviateScriptAudioProcessorEditor(WaviateScriptAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), visualizer(p.visualizer)
{
    setSize(900, 700);
    
    // Setup toolbar
    addAndMakeVisible(toolbar);
    addKeyListener(this);

    // File menu button
    toolbar.addAndMakeVisible(fileMenuButton);
    fileMenuButton.onClick = [this] { showFileMenu(); };

    // Help menu button
    toolbar.addAndMakeVisible(helpMenuButton);
    helpMenuButton.onClick = [this] { 
        juce::PopupMenu helpMenu;
        helpMenu.addItem(1, "About Waviate Script");
        helpMenu.addItem(2, "Online Documentation");
        helpMenu.addItem(3, "Keyboard Shortcuts");
        
        helpMenu.showMenuAsync(juce::PopupMenu::Options()
            .withTargetComponent(&helpMenuButton),
            [](int result) {
                if (result == 1) {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, 
                        "About Waviate Script",
                        "Waviate Script - Interactive Audio Plugin Editor\n\n"
                        "Create and compile audio processing scripts in C, C++, and Rust.\n");
                } else if (result == 2) {
                    juce::URL("https://ikamondj.github.io/WaviateScript/#/").launchInDefaultBrowser();
                } else if (result == 3) {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                        "Keyboard Shortcuts",
                        "Ctrl+N - New File\n"
                        "Ctrl+O - Open File\n"
                        "Ctrl+S - Save File\n"
                        "Ctrl+Shift+S - Save As\n"
                        "Ctrl+Enter - Compile\n");
                }
            });
    };

    // File info label
    addAndMakeVisible(currentFileLabel);
    currentFileLabel.setText("No file loaded", juce::dontSendNotification);
    currentFileLabel.setJustificationType(juce::Justification::centredLeft);
    currentFileLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    currentFileLabel.setFont(juce::Font(12.0f));
    currentFileLabel.setInterceptsMouseClicks(false, false);

    // Code editor - pass processor for compilation
    addAndMakeVisible(codeEditor);
    codeEditor.setProcessor(p);

    // Empty state message
    addAndMakeVisible(emptyStateLabel);
    emptyStateLabel.setText(
        "Create a new file or open an existing one to start editing",
        juce::dontSendNotification);
    emptyStateLabel.setJustificationType(juce::Justification::centred);
    emptyStateLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    emptyStateLabel.setFont(juce::Font(16.0f));

    addAndMakeVisible(visualizer);
    visualizer.setSamplesPerBlock(5);
    showEmptyState();
    resized();
}

WaviateScriptAudioProcessorEditor::~WaviateScriptAudioProcessorEditor() = default;

//==============================================================================
void WaviateScriptAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::lightslategrey);
}

void WaviateScriptAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    auto toolbarArea = area.removeFromTop(toolbarHeight);
    toolbar.setBounds(toolbarArea);

    {
        auto toolbarBounds = toolbar.getLocalBounds().reduced(padding);

        fileMenuButton.setBounds(toolbarBounds.removeFromLeft(buttonWidth));
        toolbarBounds.removeFromLeft(padding);

        helpMenuButton.setBounds(toolbarBounds.removeFromLeft(buttonWidth));
        toolbarBounds.removeFromLeft(padding * 2);

        currentFileLabel.setBounds(toolbarBounds);
    }

    auto editorArea = area.removeFromLeft(area.getWidth() * 3 / 4);
    auto visualizerArea = area;

    codeEditor.setBounds(editorArea);
    visualizer.setBounds(visualizerArea);
    emptyStateLabel.setBounds(editorArea);
}

//==============================================================================
bool WaviateScriptAudioProcessorEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    // Ctrl+N: New file
    if (key.isKeyCode(juce::KeyPress::createFromDescription("ctrl+n").getKeyCode())) {
        showNewFileMenu();
        return true;
    }
    
    // Ctrl+O: Open file
    if (key.isKeyCode(juce::KeyPress::createFromDescription("ctrl+o").getKeyCode())) {
        openFile();
        return true;
    }
    
    // Ctrl+S: Save file (or SaveAs if transient)
    if (key.isKeyCode(juce::KeyPress::createFromDescription("ctrl+s").getKeyCode())) {
        saveFile();
        return true;
    }
    
    // Ctrl+Shift+S: Save As
    if (key.isKeyCode(juce::KeyPress::createFromDescription("ctrl+shift+s").getKeyCode())) {
        saveFileAs();
        return true;
    }

    return false;
}

//==============================================================================
void WaviateScriptAudioProcessorEditor::showFileMenu()
{
    juce::PopupMenu fileMenu;
    
    fileMenu.addItem(1, "New", true);
    fileMenu.addSeparator();
    fileMenu.addItem(2, "Open...", true);
    fileMenu.addSeparator();
    fileMenu.addItem(3, "Save", currentScriptFile.existsAsFile() && isFileTransient);
    fileMenu.addItem(4, "Save As...", currentScriptFile.existsAsFile());
    fileMenu.addSeparator();
    fileMenu.addItem(5, "Exit", true);

    fileMenu.showMenuAsync(juce::PopupMenu::Options()
        .withTargetComponent(&fileMenuButton),
        [this](int result) {
            switch (result) {
                case 1: showNewFileMenu(); break;
                case 2: openFile(); break;
                case 3: saveFile(); break;
                case 4: saveFileAs(); break;
                case 5: juce::JUCEApplicationBase::quit(); break;
            }
        });
}

void WaviateScriptAudioProcessorEditor::showNewFileMenu()
{
    juce::PopupMenu newMenu;
    
    // C support
    newMenu.addItem(1, "C File (.wc)", true);
    
    // C++ support
    newMenu.addItem(2, "C++ File (.wcpp)", true);
    
#ifdef WAV_SCRIPT_PREMIUM
    // Rust support (premium only)
    newMenu.addItem(3, "Rust File (.wrs)", true);
#endif

    newMenu.showMenuAsync(juce::PopupMenu::Options()
        .withTargetComponent(&fileMenuButton),
        [this](int result) {
            switch (result) {
                case 1: createNewFile(".wc"); break;
                case 2: createNewFile(".wcpp"); break;
#ifdef WAV_SCRIPT_PREMIUM
                case 3: createNewFile(".wrs"); break;
#endif
            }
        });
}

void WaviateScriptAudioProcessorEditor::createNewFile(const juce::String& languageExtension)
{
    const auto templateContent = getTemplateForLanguage(languageExtension);
    
    fileChooser = std::make_unique<juce::FileChooser>(
        "Create New " + getLanguageDisplayName(languageExtension),
        getDefaultSaveDirectory(),
        "*" + languageExtension);

    const auto flags = juce::FileBrowserComponent::saveMode 
        | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this, languageExtension, templateContent](const juce::FileChooser& chooser) {
        handleNewFileDialogResult(chooser, languageExtension, templateContent);
    });
}

void WaviateScriptAudioProcessorEditor::handleNewFileDialogResult(
    const juce::FileChooser& chooser,
    const juce::String& languageExtension,
    const juce::String& templateContent)
{
    auto file = chooser.getResult();
    fileChooser.reset();

    if (!file.getFileName().isEmpty()) {
        // Ensure correct file extension
        if (!file.getFileExtension().equalsIgnoreCase(languageExtension)) {
            file = file.getParentDirectory().getChildFile(file.getFileNameWithoutExtension() + languageExtension);
        }

        // Write template to file
        if (file.replaceWithText(templateContent)) {
            isFileTransient = false;
            codeEditor.setLanguage(languageExtension);
            loadScriptFile(file);
        }
    }
}

void WaviateScriptAudioProcessorEditor::openFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Open Waviate Script",
        getDefaultSaveDirectory(),
        "*.wc;*.wcpp;*.wrs");

    const auto flags = juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
        handleOpenFileDialogResult(chooser);
    });
}

void WaviateScriptAudioProcessorEditor::handleOpenFileDialogResult(const juce::FileChooser& chooser)
{
    auto file = chooser.getResult();
    fileChooser.reset();

    if (file.existsAsFile()) {
        isFileTransient = false;
        loadScriptFile(file);
    }
}

void WaviateScriptAudioProcessorEditor::saveFile()
{
    // If file is transient (new, never saved), always do Save As
    if (isFileTransient || !currentScriptFile.existsAsFile()) {
        saveFileAs();
        return;
    }

    // Otherwise, save to current file
    const auto fileContent = codeEditor.getText();
    if (currentScriptFile.replaceWithText(fileContent)) {
        isFileModified = false;
        updateFileLabel();
    }
}

void WaviateScriptAudioProcessorEditor::saveFileAs()
{
    // Use the language selected in CodeEditor to determine the extension
    auto selectedLanguage = codeEditor.getLanguage();
    auto saveDir = getDefaultSaveDirectory();
    auto filename = currentScriptFile.getFileName();
    
    // Determine the suggested filename based on current file and language
    if (filename.isEmpty()) {
        filename = "untitled" + selectedLanguage;
    } else {
        // Replace extension with selected language extension
        filename = currentScriptFile.getFileNameWithoutExtension() + selectedLanguage;
    }

    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Waviate Script As",
        saveDir.getChildFile(filename),
        "*" + selectedLanguage);

    const auto flags = juce::FileBrowserComponent::saveMode
        | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
        handleSaveFileDialogResult(chooser, false);
    });
}

void WaviateScriptAudioProcessorEditor::handleSaveFileDialogResult(
    const juce::FileChooser& chooser,
    bool isNewFile)
{
    auto file = chooser.getResult();
    fileChooser.reset();

    if (!file.getFileName().isEmpty()) {
        // Ensure the file has the correct extension based on selected language
        auto selectedLanguage = codeEditor.getLanguage();
        if (!file.getFileExtension().equalsIgnoreCase(selectedLanguage)) {
            file = file.getParentDirectory().getChildFile(
                file.getFileNameWithoutExtension() + selectedLanguage
            );
        }
        
        const auto fileContent = codeEditor.getText();
        
        if (file.replaceWithText(fileContent)) {
            isFileTransient = false;
            isFileModified = false;
            currentScriptFile = file;
            updateFileLabel();
            
            // Compile the file if it was successfully saved
            audioProcessor.loadProgram(file);
        }
    }
}

void WaviateScriptAudioProcessorEditor::loadScriptFile(const juce::File& file)
{
    currentScriptFile = file;
    isFileModified = false;

    // Load file content into editor
    if (file.existsAsFile()) {
        auto fileContent = file.loadFileAsString();
        codeEditor.setText(fileContent);
        codeEditor.setLanguage(file.getFileExtension());
        hideEmptyState();
        
        // Compile the loaded file
        audioProcessor.loadProgram(file);
    }

    updateFileLabel();
    repaint();
}

void WaviateScriptAudioProcessorEditor::updateFileLabel()
{
    if (currentScriptFile.existsAsFile()) {
        auto displayName = currentScriptFile.getFileName();
        if (isFileTransient) {
            displayName = "• " + displayName + " (unsaved)";
        }
        currentFileLabel.setText(displayName, juce::dontSendNotification);
    } else {
        currentFileLabel.setText("No file loaded", juce::dontSendNotification);
    }
}

void WaviateScriptAudioProcessorEditor::showEmptyState()
{
    emptyStateLabel.setVisible(true);
    codeEditor.setVisible(false);
}

void WaviateScriptAudioProcessorEditor::hideEmptyState()
{
    emptyStateLabel.setVisible(false);
    codeEditor.setVisible(true);
}

//==============================================================================
juce::String WaviateScriptAudioProcessorEditor::getTemplateForLanguage(
    const juce::String& languageExtension) const
{
    if (languageExtension == ".wc") {
        return cTemplateGen.getDefaultFileSource();
    } else if (languageExtension == ".wcpp") {
        return cppTemplateGen.getDefaultFileSource();
    }
#ifdef WAV_SCRIPT_PREMIUM
    else if (languageExtension == ".wrs") {
        return rustTemplateGen.getDefaultFileSource();
    }
#endif
    
    return "";
}

juce::String WaviateScriptAudioProcessorEditor::getLanguageDisplayName(
    const juce::String& languageExtension) const
{
    if (languageExtension == ".wc") {
        return "C File";
    } else if (languageExtension == ".wcpp") {
        return "C++ File";
    }
#ifdef WAV_SCRIPT_PREMIUM
    else if (languageExtension == ".wrs") {
        return "Rust File";
    }
#endif
    
    return "File";
}

juce::File WaviateScriptAudioProcessorEditor::getDefaultSaveDirectory() const
{
    if (currentScriptFile.existsAsFile()) {
        return currentScriptFile.getParentDirectory().getFullPathName();
    }
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
}
