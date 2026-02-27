/*
  ==============================================================================

    Simple JUCE plugin editor: top bar with Open button + current script name.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "NewFileTemplateGenerator.h"

//==============================================================================
WaviateScriptAudioProcessorEditor::WaviateScriptAudioProcessorEditor(WaviateScriptAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(600, 400);

    // Top bar container
    addAndMakeVisible(topBar);

    // New file buttons
    topBar.addAndMakeVisible(newCButton);
    newCButton.onClick = [this] { onNewCClicked(); };

    topBar.addAndMakeVisible(newCppButton);
    newCppButton.onClick = [this] { onNewCppClicked(); };

    topBar.addAndMakeVisible(newRustButton);
    newRustButton.onClick = [this] { onNewRustClicked(); };

    // Open button
    topBar.addAndMakeVisible(openButton);
    openButton.onClick = [this] { onOpenClicked(); };

    // Current script label
    topBar.addAndMakeVisible(currentScriptLabel);
    currentScriptLabel.setText("No script loaded", juce::dontSendNotification);
    currentScriptLabel.setJustificationType(juce::Justification::centredLeft);
    currentScriptLabel.setInterceptsMouseClicks(false, false);

    resized();
}

WaviateScriptAudioProcessorEditor::~WaviateScriptAudioProcessorEditor() = default;

//==============================================================================
void WaviateScriptAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colours::black);
}

void WaviateScriptAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // Top bar
    topBar.setBounds(area.removeFromTop(topBarHeight));

    auto bar = topBar.getLocalBounds().reduced(8, 6);

    // Left: New file buttons
    const int buttonW = 85;
    const int spacing = 6;

    newCButton.setBounds(bar.removeFromLeft(buttonW));
    bar.removeFromLeft(spacing);

    newCppButton.setBounds(bar.removeFromLeft(buttonW));
    bar.removeFromLeft(spacing);

    newRustButton.setBounds(bar.removeFromLeft(buttonW));
    bar.removeFromLeft(spacing);

    // Open button
    openButton.setBounds(bar.removeFromLeft(buttonW));
    bar.removeFromLeft(10);

    // Rest: current script name
    currentScriptLabel.setBounds(bar);
}

//==============================================================================
void WaviateScriptAudioProcessorEditor::onNewCClicked()
{
    const auto templateContent = cGen.getDefaultFileSource();
    createNewFileWithTemplate(".wc", templateContent);
}

void WaviateScriptAudioProcessorEditor::onNewCppClicked()
{
    const auto templateContent = cppGen.getDefaultFileSource();
    createNewFileWithTemplate(".wcpp", templateContent);
}

void WaviateScriptAudioProcessorEditor::onNewRustClicked()
{
    const auto templateContent = rustGen.getDefaultFileSource();
    createNewFileWithTemplate(".wrs", templateContent);
}

void WaviateScriptAudioProcessorEditor::createNewFileWithTemplate(const juce::String& fileExtension, const juce::String& templateContent)
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Save New Waviate Script",
        currentScriptFile.existsAsFile() ? currentScriptFile.getParentDirectory()
        : juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*" + fileExtension
    );

    const auto flags = juce::FileBrowserComponent::saveMode
        | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this, fileExtension, templateContent](const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            fileChooser.reset();

            if (!file.exists())
                return;

            // Ensure correct file extension
            if (!file.getFileExtension().toLowerCase().startsWith("."))
                file = file.withFileExtension(fileExtension);

            // Write template to file
            if (!file.replaceWithText(templateContent))
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Error",
                    "Failed to create file: " + file.getFullPathName()
                );
                return;
            }

            setLoadedScriptFile(file);
        });
}

void WaviateScriptAudioProcessorEditor::onOpenClicked()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Open Waviate Script",
        currentScriptFile.existsAsFile() ? currentScriptFile.getParentDirectory()
        : juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.wc;*.wcpp;*.wrs"
    );

    const auto flags = juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser)
        {
            // If the editor is gone, JUCE won't call this, but keeping it simple:
            auto file = chooser.getResult();

            // Release chooser ASAP
            fileChooser.reset();

            if (!file.existsAsFile())
                return;

            const auto ext = file.getFileExtension().toLowerCase();
            if (ext != ".wc" && ext != ".wcpp" && ext != ".wrs")
                return;

            setLoadedScriptFile(file);
        });
}

void WaviateScriptAudioProcessorEditor::setLoadedScriptFile(const juce::File& file)
{
    currentScriptFile = file;

    // Always-visible display name (just the filename)
    currentScriptLabel.setText("Loaded: " + file.getFileName(), juce::dontSendNotification);

    // If you want to notify the processor later, this is the hook:
    // audioProcessor.loadScriptFile(file);

    if (file.existsAsFile())
        audioProcessor.loadProgram(currentScriptFile);

    repaint();
}