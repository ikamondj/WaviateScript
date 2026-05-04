/*
  ==============================================================================

    Professional JUCE plugin editor with toolbar, file menu, and keyboard shortcuts.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr const char* waviateShaderExtension = ".wsl";
    constexpr const char* waviateShaderWildcard = "*.wsl";
}

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

    toolbar.addAndMakeVisible(viewMenuButton);
    viewMenuButton.onClick = [this] { showViewMenu(); };

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
                        "Create and compile Waviate Shading Language scripts.\n");
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

    userSettings = std::make_unique<juce::PropertiesFile>(createSettingsOptions());
    selectTheme(userSettings->getValue("theme", WaviateThemes::fallback().id), false);
    resized();
}

WaviateScriptAudioProcessorEditor::~WaviateScriptAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void WaviateScriptAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto& theme = WaviateThemes::findById(currentThemeId);
    g.fillAll(theme.windowBackground);

    auto toolbarArea = getLocalBounds().removeFromTop(toolbarHeight);
    g.setColour(theme.toolbarBackground);
    g.fillRect(toolbarArea);
    g.setColour(theme.outline);
    g.drawLine(0.0f, static_cast<float>(toolbarArea.getBottom()) - 0.5f,
               static_cast<float>(getWidth()), static_cast<float>(toolbarArea.getBottom()) - 0.5f);
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

        viewMenuButton.setBounds(toolbarBounds.removeFromLeft(buttonWidth));
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
        createNewFile();
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
    fileMenu.addItem(3, "Save", currentScriptFile.existsAsFile());
    fileMenu.addItem(4, "Save As...", currentScriptFile.existsAsFile());
    fileMenu.addSeparator();
    fileMenu.addItem(5, "Exit", true);

    fileMenu.showMenuAsync(juce::PopupMenu::Options()
        .withTargetComponent(&fileMenuButton),
        [this](int result) {
            switch (result) {
                case 1: createNewFile(); break;
                case 2: openFile(); break;
                case 3: saveFile(); break;
                case 4: saveFileAs(); break;
                case 5: juce::JUCEApplicationBase::quit(); break;
            }
        });
}

void WaviateScriptAudioProcessorEditor::showViewMenu()
{
    constexpr int themeItemBase = 1000;
    juce::PopupMenu themeMenu;

    const auto& themes = WaviateThemes::all();
    for (int i = 0; i < static_cast<int>(themes.size()); ++i)
    {
        const auto& theme = themes[static_cast<size_t>(i)];
        themeMenu.addItem(themeItemBase + i, theme.name, true, currentThemeId == theme.id);
    }

    juce::PopupMenu viewMenu;
    viewMenu.addSubMenu("Theme", themeMenu);

    viewMenu.showMenuAsync(juce::PopupMenu::Options()
        .withTargetComponent(&viewMenuButton),
        [this](int result) {
            constexpr int themeItemBase = 1000;
            const auto themeIndex = result - themeItemBase;
            const auto& themes = WaviateThemes::all();

            if (themeIndex >= 0 && themeIndex < static_cast<int>(themes.size()))
                selectTheme(themes[static_cast<size_t>(themeIndex)].id, true);
        });
}

void WaviateScriptAudioProcessorEditor::createNewFile()
{
    const auto templateContent = getDefaultShaderTemplate();
    
    fileChooser = std::make_unique<juce::FileChooser>(
        "Create New Waviate Shading Language File",
        getDefaultSaveDirectory(),
        waviateShaderWildcard);

    const auto flags = juce::FileBrowserComponent::saveMode 
        | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this, templateContent](const juce::FileChooser& chooser) {
        handleNewFileDialogResult(chooser, templateContent);
    });
}

void WaviateScriptAudioProcessorEditor::handleNewFileDialogResult(
    const juce::FileChooser& chooser,
    const juce::String& templateContent)
{
    auto file = chooser.getResult();
    fileChooser.reset();

    if (!file.getFileName().isEmpty()) {
        // Ensure correct file extension
        if (!file.getFileExtension().equalsIgnoreCase(waviateShaderExtension)) {
            file = file.getParentDirectory().getChildFile(file.getFileNameWithoutExtension() + waviateShaderExtension);
        }

        // Write template to file
        if (file.replaceWithText(templateContent)) {
            isFileTransient = false;
            loadScriptFile(file);
        }
    }
}

void WaviateScriptAudioProcessorEditor::openFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Open Waviate Shading Language File",
        getDefaultSaveDirectory(),
        waviateShaderWildcard);

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
        if (!file.getFileExtension().equalsIgnoreCase(waviateShaderExtension)) {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Unsupported File Type",
                "Waviate Script currently opens Waviate Shading Language files with the .wsl suffix."
            );
            return;
        }

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
    auto saveDir = getDefaultSaveDirectory();
    auto filename = currentScriptFile.getFileName();
    
    // Determine the suggested filename based on current file.
    if (filename.isEmpty()) {
        filename = juce::String("untitled") + waviateShaderExtension;
    } else {
        filename = currentScriptFile.getFileNameWithoutExtension() + waviateShaderExtension;
    }

    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Waviate Shading Language File As",
        saveDir.getChildFile(filename),
        waviateShaderWildcard);

    const auto flags = juce::FileBrowserComponent::saveMode
        | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
        handleSaveFileDialogResult(chooser);
    });
}

void WaviateScriptAudioProcessorEditor::handleSaveFileDialogResult(
    const juce::FileChooser& chooser)
{
    auto file = chooser.getResult();
    fileChooser.reset();

    if (!file.getFileName().isEmpty()) {
        // Ensure the file has the Waviate Shading Language extension.
        if (!file.getFileExtension().equalsIgnoreCase(waviateShaderExtension)) {
            file = file.getParentDirectory().getChildFile(
                file.getFileNameWithoutExtension() + waviateShaderExtension
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
            displayName = "* " + displayName + " (unsaved)";
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
juce::String WaviateScriptAudioProcessorEditor::getDefaultShaderTemplate() const
{
    return cppTemplateGen.getDefaultFileSource();
}

void WaviateScriptAudioProcessorEditor::selectTheme(const juce::String& themeId, bool persistSelection)
{
    applyTheme(WaviateThemes::findById(themeId), persistSelection);
}

void WaviateScriptAudioProcessorEditor::applyTheme(const WaviateTheme& theme, bool persistSelection)
{
    currentThemeId = theme.id;

    themedLookAndFeel.setColourScheme(WaviateThemes::createLookAndFeelColourScheme(theme));
    setLookAndFeel(&themedLookAndFeel);

    setColour(juce::PopupMenu::backgroundColourId, theme.panelBackground);
    setColour(juce::PopupMenu::textColourId, theme.text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, theme.accent);
    setColour(juce::PopupMenu::highlightedTextColourId, theme.accentText);

    auto applyButtonColours = [&theme](juce::TextButton& button)
    {
        button.setColour(juce::TextButton::buttonColourId, theme.widgetBackground);
        button.setColour(juce::TextButton::buttonOnColourId, theme.accent);
        button.setColour(juce::TextButton::textColourOffId, theme.text);
        button.setColour(juce::TextButton::textColourOnId, theme.accentText);
    };

    applyButtonColours(fileMenuButton);
    applyButtonColours(viewMenuButton);
    applyButtonColours(helpMenuButton);

    currentFileLabel.setColour(juce::Label::textColourId, theme.mutedText);
    emptyStateLabel.setColour(juce::Label::textColourId, theme.mutedText);
    codeEditor.setTheme(theme);
    visualizer.setColours(theme.visualizerBackground, theme.visualizerWaveform);

    if (persistSelection && userSettings != nullptr)
    {
        userSettings->setValue("theme", currentThemeId);
        userSettings->saveIfNeeded();
    }

    repaint();
}

juce::PropertiesFile::Options WaviateScriptAudioProcessorEditor::createSettingsOptions()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "WaviateScript";
    options.filenameSuffix = "settings";
    options.folderName = "WaviateScript";
    options.osxLibrarySubFolder = "Application Support";
    options.commonToAllUsers = false;
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.millisecondsBeforeSaving = 0;
    return options;
}

juce::File WaviateScriptAudioProcessorEditor::getDefaultSaveDirectory() const
{
    if (currentScriptFile.existsAsFile()) {
        return currentScriptFile.getParentDirectory().getFullPathName();
    }
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
}
