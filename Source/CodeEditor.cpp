/*
  ==============================================================================

    CodeEditor.cpp - Professional code editor with Waviate Shading Language compilation
    Created: 25 Feb 2026 2:00:23am
    Author:  ikamo

  ==============================================================================
*/

#include <array>

#include "CodeEditor.h"
#include "PluginProcessor.h"

namespace
{
    constexpr std::array<int, 10> visualizerSamplesPerBlockOptions { 2, 3, 4, 6, 8, 12, 16, 32, 64, 128 };
}

//==============================================================================
CodeEditor::CodeEditor(WaviateScriptAudioProcessor* processor)
    : audioProcessor(processor)
{
    // Editor is created lazily on first use

    // Status bar components
    addAndMakeVisible(statusBar);
    
    statusBar.addAndMakeVisible(expandLogButton);
    expandLogButton.onClick = [this] {
        isLogExpanded = !isLogExpanded;
        expandLogButton.setButtonText(isLogExpanded ? "Hide Logs" : "Show Logs");
        resized();
        repaint();
    };

    statusBar.addAndMakeVisible(expandVisualizerButton);
    expandVisualizerButton.onClick = [this] {
        isVisualizerExpanded = !isVisualizerExpanded;
        updateVisualizerButton();
        resized();
        repaint();
    };

    statusBar.addAndMakeVisible(playPauseButton);
    playPauseButton.onClick = [this] {
        if (audioProcessor == nullptr)
            return;

        audioProcessor->setProcessingEnabled(! audioProcessor->isProcessingEnabled());
        updatePlayPauseButton();
    };
    
    statusBar.addAndMakeVisible(compileButton);
    compileButton.onClick = [this] { compileCurrentSource(); };

    visualizerScaleSlider.setSliderStyle(juce::Slider::LinearVertical);
    visualizerScaleSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    visualizerScaleSlider.setRange(0.0, static_cast<double>(visualizerSamplesPerBlockOptions.size() - 1), 1.0);
    visualizerScaleSlider.setValue(visualizerSamplesPerBlockIndex, juce::dontSendNotification);
    visualizerScaleSlider.setTooltip("Samples per block");
    visualizerScaleSlider.onValueChange = [this] {
        applyVisualizerScaleIndex(juce::roundToInt(visualizerScaleSlider.getValue()));
    };

    visualizerScaleValueLabel.setJustificationType(juce::Justification::centred);
    visualizerScaleValueLabel.setInterceptsMouseClicks(false, false);

    visualizerWheelOverlay.onWheel = [this](const juce::MouseWheelDetails& wheel) {
        if (wheel.deltaY != 0.0f)
            nudgeVisualizerScale(wheel.deltaY > 0.0f ? 1 : -1);
    };
    visualizerWheelOverlay.setRepaintsOnMouseActivity(false);

    logListBox.setMultiLine(true);
    logListBox.setReadOnly(true);
    logListBox.setScrollbarsShown(true);
    addChildComponent(logListBox);
    
    document.addListener(this);
    addKeyListener(this);
    
    // Initialize autocomplete
    completionProvider = std::make_unique<CompletionProvider>();
    
    updatePlayPauseButton();
    updateVisualizerButton();
    updateVisualizerScaleLabel();
    applyTheme();
}

CodeEditor::~CodeEditor()
{
    document.removeListener(this);
}

//==============================================================================
void CodeEditor::setProcessor(WaviateScriptAudioProcessor& processor)
{
    audioProcessor = &processor;
    updatePlayPauseButton();
}

void CodeEditor::setVisualizer(juce::AudioVisualiserComponent& visualizerComponent)
{
    visualizer = &visualizerComponent;
    visualizer->setColours(activeTheme.visualizerBackground, activeTheme.visualizerWaveform);
    applyVisualizerScaleIndex(visualizerSamplesPerBlockIndex);
    addChildComponent(*visualizer);
    addChildComponent(visualizerScaleSlider);
    addChildComponent(visualizerScaleValueLabel);
    addChildComponent(visualizerWheelOverlay);
    updateVisualizerButton();
    resized();
}

void CodeEditor::setOnTextChanged(std::function<void()> callback)
{
    onTextChanged = std::move(callback);
}

void CodeEditor::setCompletionsEnabled(bool shouldBeEnabled)
{
    areCompletionsEnabledFlag = shouldBeEnabled;

    if (! areCompletionsEnabledFlag && completionMenu != nullptr)
        completionMenu->hideCompletions();
}

void CodeEditor::setFileExtension(const juce::String& extension)
{
    auto normalisedExtension = extension.trim();
    if (normalisedExtension.isEmpty())
        return;

    if (! normalisedExtension.startsWithChar('.'))
        normalisedExtension = "." + normalisedExtension;

    compilerExtension = normalisedExtension.toLowerCase();

    if (completionMenu != nullptr)
        completionMenu->hideCompletions();
}

//==============================================================================
void CodeEditor::ensureEditorCreated()
{
    if (editor != nullptr)
        return;

    editor = std::make_unique<juce::CodeEditorComponent>(document, &tokeniser);
    
    editor->setTabSize(4, true);
    editor->setFont(createCodeEditorFont());
    editor->setLineNumbersShown(true);
    
    addAndMakeVisible(*editor);
    editor->addKeyListener(this);
    
    // Create completion popup menu
    completionMenu = std::make_unique<CompletionPopupMenu>(editor.get());
    completionMenu->setVisible(false);
    completionMenu->onCompletionAccepted = [this](const CompletionItem& item) {
        handleCompletionAccepted(item);
    };
    addAndMakeVisible(*completionMenu);
    
    applyTheme();
    layoutEditorArea();
}

juce::Font CodeEditor::createCodeEditorFont() const
{
    juce::Font font { withDefaultMetrics(juce::FontOptions { codeFontHeight, juce::Font::plain }) };
    font.setTypefaceName(juce::Font::getDefaultMonospacedFontName());

    juce::StringArray fallbackFonts;
    fallbackFonts.add("Cascadia Mono");
    fallbackFonts.add("Consolas");
    fallbackFonts.add("Menlo");
    fallbackFonts.add("Monaco");
    fallbackFonts.add("Courier New");
    font.setPreferredFallbackFamilies(fallbackFonts);

    return font;
}

//==============================================================================
void CodeEditor::paint(juce::Graphics& g)
{
    // Status bar background
    g.fillAll(activeTheme.panelBackground);
    g.setColour(activeTheme.toolbarBackground);
    g.fillRect(statusBar.getBounds());
    g.setColour(activeTheme.outline);
    g.drawRect(getLocalBounds());
}

void CodeEditor::resized()
{
    auto area = getLocalBounds();
    
    const int padding = 4;
    area.reduce(padding, padding);

    juce::Rectangle<int> logArea;
    if (isLogExpanded)
        logArea = area.removeFromBottom(juce::jmin(expandedLogHeight, area.getHeight()));

    const auto visualizerHeight = getVisualizerHeight(area.getHeight());
    auto visualizerArea = area.removeFromBottom(visualizerHeight);
    if (visualizer != nullptr)
    {
        auto scaleArea = visualizerArea.removeFromRight(juce::jmin(visualizerScaleControlWidth, visualizerArea.getWidth()));
        scaleArea.reduce(3, 3);

        visualizerScaleValueLabel.setBounds(scaleArea.removeFromTop(20));
        visualizerScaleSlider.setBounds(scaleArea);
        visualizer->setBounds(visualizerArea);
        visualizerWheelOverlay.setBounds(visualizerArea);
        visualizer->setVisible(isVisualizerExpanded);
        visualizerScaleSlider.setVisible(isVisualizerExpanded);
        visualizerScaleValueLabel.setVisible(isVisualizerExpanded);
        visualizerWheelOverlay.setVisible(isVisualizerExpanded);
    }

    auto statusArea = area.removeFromBottom(juce::jmin(collapsedStatusBarHeight, area.getHeight()));
    statusBar.setBounds(statusArea);
    layoutStatusBar();

    if (isLogExpanded)
        logListBox.setBounds(logArea.reduced(2));

    logListBox.setVisible(isLogExpanded);
    
    // Editor fills remaining space
    layoutEditorArea();
}

void CodeEditor::layoutEditorArea()
{
    auto area = getLocalBounds();
    area.reduce(4, 4);
    
    if (isLogExpanded)
        area.removeFromBottom(juce::jmin(expandedLogHeight, area.getHeight()));

    area.removeFromBottom(getVisualizerHeight(area.getHeight()));
    area.removeFromBottom(juce::jmin(collapsedStatusBarHeight, area.getHeight()));
    
    if (editor != nullptr)
        editor->setBounds(area);
}

void CodeEditor::layoutStatusBar()
{
    auto bounds = statusBar.getLocalBounds().reduced(4);
    
    playPauseButton.setBounds(bounds.removeFromLeft(34));
    bounds.removeFromLeft(6);

    expandVisualizerButton.setBounds(bounds.removeFromLeft(110));
    bounds.removeFromLeft(6);

    expandLogButton.setBounds(bounds.removeFromLeft(80));
    bounds.removeFromLeft(6);
    
    // Compile button on the right
    compileButton.setBounds(bounds.removeFromRight(140));
}

int CodeEditor::getVisualizerHeight(int availableHeight) const
{
    if (! isVisualizerExpanded || visualizer == nullptr || availableHeight <= 0)
        return 0;

    return juce::jmin(juce::jlimit(minVisualizerHeight, maxVisualizerHeight, availableHeight / 4),
                      availableHeight);
}

void CodeEditor::setTheme(const WaviateTheme& theme)
{
    activeTheme = theme;
    applyTheme();
}

void CodeEditor::applyTheme()
{
    setColour(juce::PopupMenu::backgroundColourId, activeTheme.panelBackground);
    setColour(juce::PopupMenu::textColourId, activeTheme.text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, activeTheme.accent);
    setColour(juce::PopupMenu::highlightedTextColourId, activeTheme.accentText);

    auto applyButtonColours = [this](juce::TextButton& button)
    {
        button.setColour(juce::TextButton::buttonColourId, activeTheme.widgetBackground);
        button.setColour(juce::TextButton::buttonOnColourId, activeTheme.accent);
        button.setColour(juce::TextButton::textColourOffId, activeTheme.text);
        button.setColour(juce::TextButton::textColourOnId, activeTheme.accentText);
    };

    applyButtonColours(expandLogButton);
    applyButtonColours(expandVisualizerButton);
    applyButtonColours(playPauseButton);
    applyButtonColours(compileButton);

    visualizerScaleSlider.setColour(juce::Slider::backgroundColourId, activeTheme.widgetBackground);
    visualizerScaleSlider.setColour(juce::Slider::trackColourId, activeTheme.accent);
    visualizerScaleSlider.setColour(juce::Slider::thumbColourId, activeTheme.accentText);
    visualizerScaleValueLabel.setColour(juce::Label::textColourId, activeTheme.mutedText);
    visualizerScaleValueLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    logListBox.setColour(juce::TextEditor::backgroundColourId, activeTheme.editorBackground);
    logListBox.setColour(juce::TextEditor::textColourId, activeTheme.editorText);
    logListBox.setColour(juce::TextEditor::highlightColourId, activeTheme.selection);
    logListBox.setColour(juce::TextEditor::highlightedTextColourId, activeTheme.accentText);
    logListBox.setColour(juce::TextEditor::outlineColourId, activeTheme.outline);
    logListBox.setColour(juce::TextEditor::focusedOutlineColourId, activeTheme.accent);
    logListBox.setColour(juce::CaretComponent::caretColourId, activeTheme.caret);

    if (editor != nullptr)
    {
        editor->setColour(juce::CodeEditorComponent::backgroundColourId, activeTheme.editorBackground);
        editor->setColour(juce::CodeEditorComponent::defaultTextColourId, activeTheme.editorText);
        editor->setColour(juce::CaretComponent::caretColourId, activeTheme.caret);
        editor->setColour(juce::CodeEditorComponent::highlightColourId, activeTheme.selection);
        editor->setColour(juce::CodeEditorComponent::lineNumberBackgroundId, activeTheme.lineNumberBackground);
        editor->setColour(juce::CodeEditorComponent::lineNumberTextId, activeTheme.lineNumberText);
        editor->setColourScheme(WaviateThemes::createCodeColourScheme(activeTheme));
        editor->repaint();
    }

    if (visualizer != nullptr)
        visualizer->setColours(activeTheme.visualizerBackground, activeTheme.visualizerWaveform);

    repaint();
}

void CodeEditor::updateVisualizerButton()
{
    expandVisualizerButton.setButtonText(isVisualizerExpanded ? "Close Viz" : "Open Viz");
    expandVisualizerButton.setTooltip(isVisualizerExpanded ? "Hide visualizer" : "Show visualizer");

    if (visualizer != nullptr)
        visualizer->setVisible(isVisualizerExpanded);

    visualizerScaleSlider.setVisible(isVisualizerExpanded && visualizer != nullptr);
    visualizerScaleValueLabel.setVisible(isVisualizerExpanded && visualizer != nullptr);
    visualizerWheelOverlay.setVisible(isVisualizerExpanded && visualizer != nullptr);
}

void CodeEditor::updatePlayPauseButton()
{
    const bool hasProcessor = audioProcessor != nullptr;
    const bool isPlaying = hasProcessor && audioProcessor->isProcessingEnabled();

    playPauseButton.setEnabled(hasProcessor);
    playPauseButton.setButtonText(isPlaying ? "||" : ">");
    playPauseButton.setTooltip(isPlaying ? "Pause processing" : "Play processing");
}

void CodeEditor::applyVisualizerScaleIndex(int index)
{
    visualizerSamplesPerBlockIndex = juce::jlimit(0,
                                                  static_cast<int>(visualizerSamplesPerBlockOptions.size()) - 1,
                                                  index);
    visualizerScaleSlider.setValue(visualizerSamplesPerBlockIndex, juce::dontSendNotification);

    if (visualizer != nullptr)
        visualizer->setSamplesPerBlock(getVisualizerSamplesPerBlock());

    updateVisualizerScaleLabel();
}

void CodeEditor::nudgeVisualizerScale(int direction)
{
    if (direction == 0)
        return;

    applyVisualizerScaleIndex(visualizerSamplesPerBlockIndex + (direction > 0 ? 1 : -1));
}

int CodeEditor::getVisualizerSamplesPerBlock() const
{
    return visualizerSamplesPerBlockOptions[static_cast<size_t>(
        juce::jlimit(0, static_cast<int>(visualizerSamplesPerBlockOptions.size()) - 1, visualizerSamplesPerBlockIndex))];
}

void CodeEditor::updateVisualizerScaleLabel()
{
    const auto value = getVisualizerSamplesPerBlock();
    visualizerScaleValueLabel.setText(juce::String(value), juce::dontSendNotification);
    visualizerScaleValueLabel.setTooltip("Samples per block: " + juce::String(value));
    visualizerScaleSlider.setTooltip("Samples per block: " + juce::String(value));
}

//==============================================================================
bool CodeEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    // Ctrl+Enter: Compile
    if (key.getModifiers().isCtrlDown() && (key.getKeyCode() == juce::KeyPress::returnKey))
    {
        compileCurrentSource();
        return true;
    }

    if (completionMenu != nullptr && completionMenu->isOpen() && completionMenu->keyPressed(key, editor.get()))
        return true;

    const bool isCompletionOrPlaybackShortcut =
        (key.getModifiers().isCtrlDown() || key.getModifiers().isCommandDown())
        && key.getKeyCode() == juce::KeyPress::spaceKey;

    if (isCompletionOrPlaybackShortcut)
    {
        if (areCompletionsEnabledFlag && editor != nullptr && editor->hasKeyboardFocus(true))
        {
            updateCompletions();
            return true;
        }

        if (audioProcessor != nullptr)
        {
            audioProcessor->setProcessingEnabled(! audioProcessor->isProcessingEnabled());
            updatePlayPauseButton();
            return true;
        }
    }
    
    // Ctrl+L: Toggle log viewer
    if (key.getModifiers().isCtrlDown() && (key.getKeyCode() == juce::KeyPress::createFromDescription("ctrl+l").getKeyCode()))
    {
        isLogExpanded = !isLogExpanded;
        expandLogButton.setButtonText(isLogExpanded ? "Hide Logs" : "Show Logs");
        resized();
        repaint();
        return true;
    }
    
    // Ctrl+W: Toggle waveform visualizer
    if (key.getModifiers().isCtrlDown() && (key.getKeyCode() == juce::KeyPress::createFromDescription("ctrl+w").getKeyCode()))
    {
        isVisualizerExpanded = !isVisualizerExpanded;
        updateVisualizerButton();
        resized();
        repaint();
        return true;
    }

    return false;
}

void CodeEditor::codeDocumentTextInserted(const juce::String& newText, int)
{
    if (onTextChanged != nullptr)
        onTextChanged();
    
    // Trigger autocompletion for single character inserts
    if (newText.length() == 1) {
        triggerAutocompletionIfApplicable(newText[0]);
    }
}

void CodeEditor::codeDocumentTextDeleted(int, int)
{
    if (onTextChanged != nullptr)
        onTextChanged();
    
    // Hide completions if they're open
    if (completionMenu != nullptr && completionMenu->isOpen())
        completionMenu->hideCompletions();
}

//==============================================================================
void CodeEditor::setText(const juce::String& text)
{
    ensureEditorCreated();
    document.replaceAllContent(text);
}

juce::String CodeEditor::getText() const
{
    return document.getAllContent();
}

//==============================================================================
void CodeEditor::compileCurrentSource()
{
    addLogMessage("Compiling Waviate Shading Language...");
    performCompilation();
}

void CodeEditor::performCompilation()
{
    if (audioProcessor == nullptr)
    {
        addLogMessage("Error: Processor not initialized");
        return;
    }
    
    const auto sourceCode = getText();

    const auto result = audioProcessor->compileAndActivateSource(compilerExtension, sourceCode);
    if (! result)
    {
        addLogMessage("Compilation error: " + result.errorMessage);
        return;
    }

    if (! result.hasSampleShader && ! result.hasFrequencyShader)
    {
        addLogMessage("Compilation failed - no sample_process or frequency_process entry point was emitted");
        return;
    }

    juce::String enabledStages;
    if (result.hasSampleShader)
        enabledStages << "sample";
    if (result.hasFrequencyShader)
    {
        if (enabledStages.isNotEmpty())
            enabledStages << " + ";
        enabledStages << "frequency";
    }

    addLogMessage("Compilation successful (" + enabledStages
        + (enabledStages.contains("+") ? " stages enabled)" : " stage enabled)"));
}

//==============================================================================
void CodeEditor::addLogMessage(const juce::String& message)
{
    logMessages.push_back(message);
    
    // Keep log size reasonable
    if (logMessages.size() > 100)
        logMessages.erase(logMessages.begin());

    juce::String combinedLog;
    for (const auto& logMessage : logMessages)
        combinedLog << logMessage << juce::newLine;

    logListBox.setText(combinedLog, juce::dontSendNotification);
    logListBox.moveCaretToEnd();
}

void CodeEditor::clearLog()
{
    logMessages.clear();
    logListBox.clear();
}

//==============================================================================
void CodeEditor::updateCompletions()
{
    if (editor == nullptr || completionProvider == nullptr || completionMenu == nullptr)
        return;

    if (! areCompletionsEnabledFlag || ! compilerExtension.equalsIgnoreCase(".wlsl"))
    {
        completionMenu->hideCompletions();
        return;
    }

    const auto sourceCode = getText();
    const auto caretPos = editor->getCaretPos();
    const int caretOffset = caretPos.getPosition();

    const auto completions = completionProvider->getCompletions(sourceCode, caretOffset);

    if (completions.empty())
    {
        completionMenu->hideCompletions();
    }
    else
    {
        completionMenu->showCompletions(completions);
    }
}

void CodeEditor::triggerAutocompletionIfApplicable(juce::juce_wchar createdChar)
{
    if (editor == nullptr || completionProvider == nullptr || completionMenu == nullptr)
        return;

    if (! areCompletionsEnabledFlag)
    {
        completionMenu->hideCompletions();
        return;
    }

    // Show completion menu after typing alphanumeric, underscore, or member access operators.
    const bool isIdentifierChar = juce::CharacterFunctions::isLetterOrDigit(createdChar) || createdChar == '_';
    bool isMemberAccess = createdChar == '.';

    if (createdChar == '>')
    {
        const auto sourceCode = getText();
        const auto caretOffset = editor->getCaretPos().getPosition();
        isMemberAccess = caretOffset >= 2 && sourceCode[caretOffset - 2] == '-';
    }

    if (isIdentifierChar || isMemberAccess)
    {
        updateCompletions();
    }
    else
    {
        // Hide completions for other characters
        completionMenu->hideCompletions();
    }
}

void CodeEditor::handleCompletionAccepted(const CompletionItem& item)
{
    if (editor == nullptr)
        return;

    const auto caretPos = editor->getCaretPos();
    const int caretOffset = caretPos.getPosition();
    const auto sourceCode = getText();

    const auto insertionText = item.insertText.isNotEmpty() ? item.insertText : item.name;
    if (insertionText.isEmpty())
        return;

    // Extract prefix (word being completed)
    int prefixStart = caretOffset;
    while (prefixStart > 0 && (juce::CharacterFunctions::isLetterOrDigit(sourceCode[prefixStart - 1]) || sourceCode[prefixStart - 1] == '_'))
        --prefixStart;

    const int prefixLength = caretOffset - prefixStart;

    // Delete prefix and insert completion
    document.deleteSection(prefixStart, prefixLength);
    document.insertText(prefixStart, insertionText);

    // Position caret after insertion
    const int newCaretOffset = juce::jlimit(0,
                                            document.getAllContent().length(),
                                            prefixStart + insertionText.length() + item.cursorOffsetAfterInsert);
    const juce::CodeDocument::Position newPos(document, newCaretOffset);
    editor->moveCaretTo(newPos, false);

    // Hide completion menu
    if (completionMenu != nullptr)
        completionMenu->hideCompletions();
}
