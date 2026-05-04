/*
  ==============================================================================

    CodeEditor.cpp - Professional code editor with Waviate Shading Language compilation
    Created: 25 Feb 2026 2:00:23am
    Author:  ikamo

  ==============================================================================
*/

#include "CodeEditor.h"
#include "PluginProcessor.h"

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

    logListBox.setMultiLine(true);
    logListBox.setReadOnly(true);
    logListBox.setScrollbarsShown(true);
    addChildComponent(logListBox);
    
    addKeyListener(this);
    updateVisualizerButton();
    document.addListener(this);
    addKeyListener(this);
    
    // Initialize autocomplete
    completionProvider = std::make_unique<CompletionProvider>();
    
    updatePlayPauseButton();
    updateVisualizerButton();
    applyTheme();
}

CodeEditor::~CodeEditor() = default;

//==============================================================================
void CodeEditor::setProcessor(WaviateScriptAudioProcessor& processor)
{
    audioProcessor = &processor;
    updatePlayPauseButton();
}

void CodeEditor::setVisualizer(juce::AudioVisualiserComponent& visualizerComponent)
{
    visualizer = &visualizerComponent;
    visualizer->setSamplesPerBlock(5);
    visualizer->setColours(activeTheme.visualizerBackground, activeTheme.visualizerWaveform);
    addChildComponent(*visualizer);
    updateVisualizerButton();
    resized();
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
    const auto visualizerArea = area.removeFromBottom(visualizerHeight);
    if (visualizer != nullptr)
    {
        visualizer->setBounds(visualizerArea);
        visualizer->setVisible(isVisualizerExpanded);
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
}

void CodeEditor::updatePlayPauseButton()
{
    const bool hasProcessor = audioProcessor != nullptr;
    const bool isPlaying = hasProcessor && audioProcessor->isProcessingEnabled();

    playPauseButton.setEnabled(hasProcessor);
    playPauseButton.setButtonText(isPlaying ? "||" : ">");
    playPauseButton.setTooltip(isPlaying ? "Pause processing" : "Play processing");
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
    
    // Ctrl+Space: Toggle play/pause (handled by PluginEditor)
    // Note: This is handled by the parent component, so we don't consume it here
    
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
    
    // Ctrl+Space: Show autocomplete (when editor has focus)
    if (key.getModifiers().isCtrlDown() && (key.getKeyCode() == juce::KeyPress::spaceKey))
    {
        updateCompletions();
        return true;
    }
    
    return false;
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
    
    try
    {
        auto it = audioProcessor->compilers.find(compilerExtension.toStdString());
        if (it == audioProcessor->compilers.end())
        {
            addLogMessage("Error: Waviate Shading Language compiler not found");
            return;
        }
        
        SampleShader outSample = nullptr;
        FrequencyShader outFrequency = nullptr;
        
        it->second->compileSource(sourceCode.toStdString(), outSample, outFrequency);
        
        if (outSample != nullptr || outFrequency != nullptr)
        {
            // Store compiled functions in processor's atomic pointers
            audioProcessor->activeSampleShader.store(outSample, std::memory_order_release);
            audioProcessor->activeFrequencyShader.store(outFrequency, std::memory_order_release);

            juce::String enabledStages;
            if (outSample != nullptr)
                enabledStages << "sample";
            if (outFrequency != nullptr)
            {
                if (enabledStages.isNotEmpty())
                    enabledStages << " + ";
                enabledStages << "frequency";
            }

            addLogMessage("Compilation successful (" + enabledStages
                + (enabledStages.contains("+") ? " stages enabled)" : " stage enabled)"));
        }
        else
        {
            addLogMessage("Compilation failed - no sample_process or frequency_process entry point was emitted");
        }
    }
    catch (const std::exception& e)
    {
        addLogMessage("Compilation error: " + juce::String(e.what()));
    }
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
// Autocomplete methods
void CodeEditor::updateCompletions()
{
    if (editor == nullptr || completionProvider == nullptr || completionMenu == nullptr)
        return;
    
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
        completionMenu->showCompletions(completions, *editor, caretOffset);
    }
}

void CodeEditor::triggerAutocompletionIfApplicable(juce::juce_wchar createdChar)
{
    // Show completion on alphanumeric, underscore, or member access operators
    if (std::isalnum(createdChar) || createdChar == '_' || createdChar == '.' || createdChar == '>')
    {
        updateCompletions();
    }
    else
    {
        // Hide completion menu for other characters
        if (completionMenu != nullptr)
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
    
    // Extract prefix (word being completed)
    int prefixStart = caretOffset;
    while (prefixStart > 0 && (std::isalnum(sourceCode[prefixStart - 1]) || sourceCode[prefixStart - 1] == '_'))
        prefixStart--;
    
    const int prefixLength = caretOffset - prefixStart;
    
    // Delete prefix and insert completion
    document.deleteSection(prefixStart, prefixLength);
    document.insertText(prefixStart, item.insertText);
    
    // Position caret after insertion
    const int newCaretOffset = prefixStart + item.insertText.length() + item.cursorOffsetAfterInsert;
    const juce::CodeDocument::Position newPos(document, newCaretOffset);
    editor->moveCaretTo(newPos, false);
    
    // Hide completion menu
    if (completionMenu != nullptr)
        completionMenu->hideCompletions();
}

//==============================================================================
// CodeDocument::Listener implementation
void CodeEditor::codeDocumentTextInserted(const juce::String& newText, int insertIndex)
{
    // Trigger autocompletion for single character inserts
    if (newText.length() == 1)
    {
        triggerAutocompletionIfApplicable(newText[0]);
    }
}

void CodeEditor::codeDocumentTextDeleted(int startIndex, int endIndex)
{
    // Hide completion menu when text is deleted
    if (completionMenu != nullptr)
        completionMenu->hideCompletions();
}
