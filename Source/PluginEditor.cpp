/*
  ==============================================================================

    Professional JUCE plugin editor with toolbar, file menu, and keyboard shortcuts.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <array>

namespace
{
    constexpr const char* waviateShaderExtension = ".wlsl";
    constexpr const char* waviateShaderWildcard = "*.wlsl";
    constexpr const char* lastOpenedFileSettingKey = "lastOpenedFile";
    constexpr const char* recentFilesSettingKey = "recentFiles";
    constexpr const char* codeCompletionsSettingKey = "codeCompletionsEnabled";
    constexpr const char* fuelLimitPresetSettingKey = "fuelLimitPreset";
    constexpr int recentFileItemBase = 2000;
    constexpr int fuelLimitItemBase = 3000;
    constexpr int maxRecentFileCount = 10;

    constexpr std::array<waviate::compile::FuelLimitPreset, 5> fuelLimitMenuPresets {
        waviate::compile::FuelLimitPreset::Minimal,
        waviate::compile::FuelLimitPreset::Low,
        waviate::compile::FuelLimitPreset::Medium,
        waviate::compile::FuelLimitPreset::High,
        waviate::compile::FuelLimitPreset::Massive
    };
}

//==============================================================================
WaviateScriptAudioProcessorEditor::WaviateScriptAudioProcessorEditor(WaviateScriptAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      audioClipsPanel(p, [this] { codeEditor.compileCurrentSource(); })
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

    toolbar.addAndMakeVisible(toolsMenuButton);
    toolsMenuButton.onClick = [this] { showToolsMenu(); };

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
                        "Ctrl+Enter - Compile\n"
                        "Ctrl+Space - Play/Pause (Autocomplete in editor)\n"
                        "Ctrl+L - Toggle Log Viewer\n"
                        "Ctrl+W - Toggle Waveform Visualizer\n\n"
                        "Features:\n"
                        "• Autocomplete: Type to see C++ and Waviate API suggestions\n"
                        "• Member access: Type 'input->' or 'wav.' for member completions\n");
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
    codeEditor.setVisualizer(p.visualizer);
    codeEditor.setOnTextChanged([this] { markFileModified(); });

    // Empty state message
    addAndMakeVisible(emptyStateLabel);
    emptyStateLabel.setText(
        "Create a new file or open an existing one to start editing",
        juce::dontSendNotification);
    emptyStateLabel.setJustificationType(juce::Justification::centred);
    emptyStateLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    emptyStateLabel.setFont(juce::Font(16.0f));

    showEmptyState();

    userSettings = std::make_unique<juce::PropertiesFile>(createSettingsOptions());
    setFuelLimitPreset(
        waviate::compile::fuelLimitPresetFromId(
            userSettings->getValue(
                fuelLimitPresetSettingKey,
                juce::String(waviate::compile::fuelLimitPresetId(waviate::compile::FuelLimitPreset::Medium).data())).toStdString()),
        false);
    setCompletionsEnabled(userSettings->getBoolValue(codeCompletionsSettingKey, true), false);
    selectTheme(userSettings->getValue("theme", WaviateThemes::fallback().id), false);
    loadLastOpenedFileIfAvailable();
    
    addChildComponent(audioClipsPanel);
    audioProcessor.onAudioCacheChanged = [this]() {
        audioClipsPanel.updateList();
    };
    startTimer(100);

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

        toolsMenuButton.setBounds(toolbarBounds.removeFromLeft(buttonWidth));
        toolbarBounds.removeFromLeft(padding);

        helpMenuButton.setBounds(toolbarBounds.removeFromLeft(buttonWidth));
        toolbarBounds.removeFromLeft(padding * 2);

        currentFileLabel.setBounds(toolbarBounds);
    }

    if (isAudioClipsPanelOpen)
    {
        auto clipsArea = area.removeFromRight(250);
        audioClipsPanel.setBounds(clipsArea);
        audioClipsPanel.setVisible(true);
    }
    else
    {
        audioClipsPanel.setVisible(false);
    }

    codeEditor.setBounds(area);
    emptyStateLabel.setBounds(area);
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

void WaviateScriptAudioProcessorEditor::userTriedToCloseWindow()
{
    requestApplicationQuit();
}

//==============================================================================
void WaviateScriptAudioProcessorEditor::showFileMenu()
{
    juce::PopupMenu fileMenu;
    juce::PopupMenu recentMenu;
    auto recentFiles = getRecentFiles();

    if (recentFiles.isEmpty())
    {
        recentMenu.addItem(recentFileItemBase, "No Recent Files", false);
    }
    else
    {
        for (int i = 0; i < recentFiles.size(); ++i)
        {
            const juce::File file(recentFiles[i]);
            recentMenu.addItem(recentFileItemBase + i,
                               file.getFileName() + "    " + file.getParentDirectory().getFullPathName());
        }
    }
    
    fileMenu.addItem(1, "New", true);
    fileMenu.addSeparator();
    fileMenu.addItem(2, "Open...", true);
    fileMenu.addSubMenu("Open Recent", recentMenu, ! recentFiles.isEmpty());
    fileMenu.addSeparator();
    fileMenu.addItem(3, "Save", currentScriptFile.existsAsFile());
    fileMenu.addItem(4, "Save As...", currentScriptFile.existsAsFile());
    fileMenu.addSeparator();
    fileMenu.addItem(5, "Exit", true);

    fileMenu.showMenuAsync(juce::PopupMenu::Options()
        .withTargetComponent(&fileMenuButton),
        [this](int result) {
            if (result >= recentFileItemBase && result < recentFileItemBase + maxRecentFileCount)
            {
                openRecentFile(result - recentFileItemBase);
                return;
            }

            switch (result) {
                case 1: createNewFile(); break;
                case 2: openFile(); break;
                case 3: saveFile(); break;
                case 4: saveFileAs(); break;
                case 5: requestApplicationQuit(); break;
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
    viewMenu.addItem(1, "Show Audio Clips", true, isAudioClipsPanelOpen);

    viewMenu.showMenuAsync(juce::PopupMenu::Options()
        .withTargetComponent(&viewMenuButton),
        [this](int result) {
            if (result == 1)
            {
                isAudioClipsPanelOpen = ! isAudioClipsPanelOpen;
                if (isAudioClipsPanelOpen)
                    audioClipsPanel.updateList();
                resized();
            }
            else
            {
                constexpr int themeItemBase = 1000;
                const auto themeIndex = result - themeItemBase;
                const auto& themes = WaviateThemes::all();

                if (themeIndex >= 0 && themeIndex < static_cast<int>(themes.size()))
                    selectTheme(themes[static_cast<size_t>(themeIndex)].id, true);
            }
        });
}

void WaviateScriptAudioProcessorEditor::showToolsMenu()
{
    constexpr int completionsItemId = 1;

    juce::PopupMenu toolsMenu;
    juce::PopupMenu fuelLimitMenu;

    const auto activeFuelLimit = audioProcessor.getFuelLimitPreset();
    for (int i = 0; i < static_cast<int>(fuelLimitMenuPresets.size()); ++i)
    {
        const auto preset = fuelLimitMenuPresets[static_cast<size_t>(i)];
        fuelLimitMenu.addItem(
            fuelLimitItemBase + i,
            juce::String(waviate::compile::fuelLimitPresetDisplayName(preset).data()),
            true,
            activeFuelLimit == preset);
    }

    toolsMenu.addItem(completionsItemId, "Completions", true, codeEditor.areCompletionsEnabled());
    toolsMenu.addSubMenu("Fuel Limits", fuelLimitMenu);

    toolsMenu.showMenuAsync(juce::PopupMenu::Options()
        .withTargetComponent(&toolsMenuButton),
        [this](int result) {
            if (result == completionsItemId)
            {
                setCompletionsEnabled(! codeEditor.areCompletionsEnabled(), true);
                return;
            }

            const auto fuelLimitIndex = result - fuelLimitItemBase;
            if (fuelLimitIndex >= 0 && fuelLimitIndex < static_cast<int>(fuelLimitMenuPresets.size()))
                setFuelLimitPreset(fuelLimitMenuPresets[static_cast<size_t>(fuelLimitIndex)], true);
        });
}

void WaviateScriptAudioProcessorEditor::createNewFile()
{
    runAfterSavePrompt([this] {
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
    runAfterSavePrompt([this] {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Open Waviate Shading Language File",
            getDefaultSaveDirectory(),
            waviateShaderWildcard);

        const auto flags = juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
            handleOpenFileDialogResult(chooser);
        });
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
                "Waviate Script currently opens Waviate Shading Language files with the .wlsl suffix."
            );
            return;
        }

        isFileTransient = false;
        loadScriptFile(file);
    }
}

void WaviateScriptAudioProcessorEditor::saveFile()
{
    saveFileThen({});
}

void WaviateScriptAudioProcessorEditor::saveFileThen(std::function<void()> afterSave)
{
    // If file is transient (new, never saved), always do Save As
    if (isFileTransient || !currentScriptFile.existsAsFile()) {
        saveFileAsThen(std::move(afterSave));
        return;
    }

    // Otherwise, save to current file
    const auto fileContent = codeEditor.getText();
    if (currentScriptFile.replaceWithText(fileContent)) {
        isFileModified = false;
        rememberRecentFile(currentScriptFile);
        updateFileLabel();

        if (afterSave != nullptr)
            afterSave();
    }
}

void WaviateScriptAudioProcessorEditor::saveFileAs()
{
    saveFileAsThen({});
}

void WaviateScriptAudioProcessorEditor::saveFileAsThen(std::function<void()> afterSave)
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

    fileChooser->launchAsync(flags, [this, afterSave = std::move(afterSave)](const juce::FileChooser& chooser) mutable {
        handleSaveFileDialogResult(chooser, std::move(afterSave));
    });
}

void WaviateScriptAudioProcessorEditor::handleSaveFileDialogResult(
    const juce::FileChooser& chooser,
    std::function<void()> afterSave)
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
            rememberRecentFile(file);
            updateFileLabel();
            
            // Compile the file if it was successfully saved
            audioProcessor.loadProgram(file);

            if (afterSave != nullptr)
                afterSave();
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
        isFileModified = false;
        hideEmptyState();
        
        // Compile the loaded file
        audioProcessor.loadProgram(file);
        rememberRecentFile(file);
    }

    updateFileLabel();
    repaint();
}

void WaviateScriptAudioProcessorEditor::openRecentFile(int index)
{
    auto recentFiles = getRecentFiles();
    if (index < 0 || index >= recentFiles.size())
        return;

    const juce::File file(recentFiles[index]);
    if (! file.existsAsFile())
    {
        recentFiles.remove(index);
        storeRecentFiles(recentFiles);
        clearMissingLastOpenedFile();
        return;
    }

    runAfterSavePrompt([this, file] {
        isFileTransient = false;
        loadScriptFile(file);
    });
}

void WaviateScriptAudioProcessorEditor::loadLastOpenedFileIfAvailable()
{
    if (userSettings == nullptr)
        return;

    const auto path = userSettings->getValue(lastOpenedFileSettingKey);
    if (path.isEmpty())
        return;

    const juce::File file(path);
    if (file.existsAsFile() && file.getFileExtension().equalsIgnoreCase(waviateShaderExtension))
    {
        isFileTransient = false;
        loadScriptFile(file);
        return;
    }

    clearMissingLastOpenedFile();
}

void WaviateScriptAudioProcessorEditor::persistLastOpenedFile(const juce::File& file)
{
    if (userSettings == nullptr || ! file.existsAsFile())
        return;

    userSettings->setValue(lastOpenedFileSettingKey, file.getFullPathName());
    userSettings->saveIfNeeded();
}

void WaviateScriptAudioProcessorEditor::clearMissingLastOpenedFile()
{
    if (userSettings == nullptr)
        return;

    const auto path = userSettings->getValue(lastOpenedFileSettingKey);
    if (path.isNotEmpty() && ! juce::File(path).existsAsFile())
    {
        userSettings->removeValue(lastOpenedFileSettingKey);
        userSettings->saveIfNeeded();
    }
}

juce::StringArray WaviateScriptAudioProcessorEditor::getRecentFiles()
{
    juce::StringArray files;
    if (userSettings != nullptr)
        files.addLines(userSettings->getValue(recentFilesSettingKey));

    juce::StringArray filtered;
    for (const auto& path : files)
    {
        const juce::File file(path);
        if (path.isNotEmpty()
            && file.existsAsFile()
            && file.getFileExtension().equalsIgnoreCase(waviateShaderExtension)
            && ! filtered.contains(path, true))
        {
            filtered.add(path);
        }
    }

    if (filtered.size() != files.size())
        storeRecentFiles(filtered);

    return filtered;
}

void WaviateScriptAudioProcessorEditor::storeRecentFiles(const juce::StringArray& files)
{
    if (userSettings == nullptr)
        return;

    userSettings->setValue(recentFilesSettingKey, files.joinIntoString("\n"));
    userSettings->saveIfNeeded();
}

void WaviateScriptAudioProcessorEditor::rememberRecentFile(const juce::File& file)
{
    if (userSettings == nullptr || ! file.existsAsFile())
        return;

    auto recentFiles = getRecentFiles();
    const auto path = file.getFullPathName();

    for (int i = recentFiles.size(); --i >= 0;)
        if (recentFiles[i].equalsIgnoreCase(path))
            recentFiles.remove(i);

    recentFiles.insert(0, path);

    while (recentFiles.size() > maxRecentFileCount)
        recentFiles.remove(recentFiles.size() - 1);

    storeRecentFiles(recentFiles);
    persistLastOpenedFile(file);
}

void WaviateScriptAudioProcessorEditor::markFileModified()
{
    if (! codeEditor.isVisible())
        return;

    if (! isFileModified)
    {
        isFileModified = true;
        updateFileLabel();
    }
}

void WaviateScriptAudioProcessorEditor::runAfterSavePrompt(std::function<void()> action)
{
    if (! isFileModified)
    {
        if (action != nullptr)
            action();
        return;
    }

    const auto fileName = currentScriptFile.existsAsFile()
        ? currentScriptFile.getFileName()
        : juce::String("this file");

    juce::AlertWindow::showYesNoCancelBox(
        juce::MessageBoxIconType::QuestionIcon,
        "Save Changes?",
        "Save changes to " + fileName + " before continuing?",
        "Save",
        "Discard",
        "Cancel",
        this,
        juce::ModalCallbackFunction::create([this, action = std::move(action)](int result) mutable {
            if (result == 1)
                saveFileThen(std::move(action));
            else if (result == 2 && action != nullptr)
                action();
        }));
}

void WaviateScriptAudioProcessorEditor::requestApplicationQuit()
{
    runAfterSavePrompt([] {
        juce::JUCEApplicationBase::quit();
    });
}

void WaviateScriptAudioProcessorEditor::updateFileLabel()
{
    if (currentScriptFile.existsAsFile()) {
        auto displayName = currentScriptFile.getFileName();
        if (isFileModified || isFileTransient)
            displayName = "* " + displayName;

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
    applyButtonColours(toolsMenuButton);
    applyButtonColours(helpMenuButton);

    currentFileLabel.setColour(juce::Label::textColourId, theme.mutedText);
    emptyStateLabel.setColour(juce::Label::textColourId, theme.mutedText);
    codeEditor.setTheme(theme);

    if (persistSelection && userSettings != nullptr)
    {
        userSettings->setValue("theme", currentThemeId);
        userSettings->saveIfNeeded();
    }

    repaint();
}

void WaviateScriptAudioProcessorEditor::setCompletionsEnabled(bool shouldBeEnabled, bool persistSelection)
{
    codeEditor.setCompletionsEnabled(shouldBeEnabled);

    if (persistSelection && userSettings != nullptr)
    {
        userSettings->setValue(codeCompletionsSettingKey, shouldBeEnabled);
        userSettings->saveIfNeeded();
    }
}

void WaviateScriptAudioProcessorEditor::setFuelLimitPreset(waviate::compile::FuelLimitPreset preset, bool persistSelection)
{
    audioProcessor.setFuelLimitPreset(preset);

    if (persistSelection && userSettings != nullptr)
    {
        userSettings->setValue(
            fuelLimitPresetSettingKey,
            juce::String(waviate::compile::fuelLimitPresetId(preset).data()));
        userSettings->saveIfNeeded();
    }

    if (persistSelection)
    {
        if (codeEditor.isVisible())
            codeEditor.compileCurrentSource();
        else if (currentScriptFile.existsAsFile())
            audioProcessor.loadProgram(currentScriptFile);
    }
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

void WaviateScriptAudioProcessorEditor::timerCallback()
{
    if (isAudioClipsPanelOpen)
    {
        audioClipsPanel.updateList();
    }
}
