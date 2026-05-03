/*
  ==============================================================================

    CodeEditor.cpp - Professional code editor with language selection and compilation
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
    
    // Language selector dropdown
    languageSelector = std::make_unique<juce::ComboBox>();
    languageSelector->addItem("C", 1);
    languageSelector->addItem("C++", 2);
#ifdef WAV_SCRIPT_PREMIUM
    languageSelector->addItem("Rust", 3);
#endif
    languageSelector->setSelectedItemIndex(1);  // Default to C++
    languageSelector->onChange = [this] { 
        onLanguageSelected(languageSelector->getSelectedItemIndex());
    };
    addAndMakeVisible(*languageSelector);
    
    // Status bar components
    addAndMakeVisible(statusBar);
    
    statusBar.addAndMakeVisible(expandLogButton);
    expandLogButton.onClick = [this] {
        isLogExpanded = !isLogExpanded;
        expandLogButton.setButtonText(isLogExpanded ? "▲ Logs" : "▼ Logs");
        resized();
        repaint();
    };
    
    statusBar.addAndMakeVisible(compileButton);
    compileButton.onClick = [this] { compileCurrentSource(); };
    
    addKeyListener(this);
}

CodeEditor::~CodeEditor() = default;

//==============================================================================
void CodeEditor::setProcessor(WaviateScriptAudioProcessor& processor)
{
    audioProcessor = &processor;
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
    editor->setColour(juce::CodeEditorComponent::backgroundColourId, 
                      juce::Colours::lightslategrey.brighter());
    editor->setColour(juce::CodeEditorComponent::defaultTextColourId, 
                      juce::Colours::whitesmoke);
    editor->setColour(juce::CaretComponent::caretColourId, 
                      juce::Colours::whitesmoke);
    editor->setColour(juce::CodeEditorComponent::highlightColourId, 
                      juce::Colours::cornflowerblue.withAlpha(0.25f));
    editor->setColour(juce::CodeEditorComponent::lineNumberBackgroundId,
                      juce::Colours::darkgrey.withAlpha(0.5f));
    
    addAndMakeVisible(*editor);
    editor->addKeyListener(this);
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
    g.fillAll(juce::Colours::darkslategrey);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.8f));
    g.fillRect(statusBar.getBounds());
}

void CodeEditor::resized()
{
    auto area = getLocalBounds();
    
    // Language selector in top right
    const int selectorWidth = 120;
    const int selectorHeight = 24;
    const int padding = 4;
    languageSelector->setBounds(
        area.getRight() - selectorWidth - padding,
        padding,
        selectorWidth,
        selectorHeight
    );
    
    // Status bar at bottom
    const int statusBarHeight = isLogExpanded ? 
        collapsedStatusBarHeight + expandedLogHeight : 
        collapsedStatusBarHeight;
    
    auto statusArea = area.removeFromBottom(statusBarHeight);
    statusBar.setBounds(statusArea);
    layoutStatusBar();
    
    // Log area (if expanded)
    if (isLogExpanded && statusBarHeight > collapsedStatusBarHeight)
    {
        auto logArea = statusArea.removeFromBottom(expandedLogHeight);
        logListBox.setBounds(logArea.reduced(2));
    }
    
    // Editor fills remaining space
    layoutEditorArea();
}

void CodeEditor::layoutEditorArea()
{
    auto area = getLocalBounds();
    
    // Reserve space for language selector
    area.removeFromRight(130);
    area.removeFromTop(30);
    
    // Reserve space for status bar
    const int statusBarHeight = isLogExpanded ? 
        collapsedStatusBarHeight + expandedLogHeight : 
        collapsedStatusBarHeight;
    area.removeFromBottom(statusBarHeight);
    
    if (editor != nullptr)
        editor->setBounds(area);
}

void CodeEditor::layoutStatusBar()
{
    auto bounds = statusBar.getLocalBounds().reduced(4);
    
    // Expand button on the left
    expandLogButton.setBounds(bounds.removeFromLeft(80));
    bounds.removeFromLeft(6);
    
    // Compile button on the right
    compileButton.setBounds(bounds.removeFromRight(140));
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

void CodeEditor::setLanguage(const juce::String& languageExtension)
{
    currentLanguage = languageExtension;
    updateLanguageSelector();
}

juce::String CodeEditor::getLanguage() const
{
    return currentLanguage;
}

void CodeEditor::updateLanguageSelector()
{
    if (!languageSelector)
        return;
    
    if (currentLanguage == ".wc")
        languageSelector->setSelectedItemIndex(0);
    else if (currentLanguage == ".wcpp")
        languageSelector->setSelectedItemIndex(1);
#ifdef WAV_SCRIPT_PREMIUM
    else if (currentLanguage == ".wrs")
        languageSelector->setSelectedItemIndex(2);
#endif
}

void CodeEditor::onLanguageSelected(int languageIndex)
{
#ifdef WAV_SCRIPT_PREMIUM
    // Premium: all languages available
    if (languageIndex == 0)
        currentLanguage = ".wc";
    else if (languageIndex == 1)
        currentLanguage = ".wcpp";
    else if (languageIndex == 2)
        currentLanguage = ".wrs";
#else
    // Non-premium: Rust disabled
    if (languageIndex == 0)
        currentLanguage = ".wc";
    else if (languageIndex == 1)
        currentLanguage = ".wcpp";
    else if (languageIndex == 2)
    {
        // Prevent selection of Rust
        languageSelector->setSelectedItemIndex(1);
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "Premium Feature",
            "Rust support is available in Waviate Script Premium edition only."
        );
    }
#endif
}

//==============================================================================
void CodeEditor::compileCurrentSource()
{
    addLogMessage("Compiling " + currentLanguage + "...");
    performCompilation();
}

void CodeEditor::performCompilation()
{
    if (audioProcessor == nullptr)
    {
        addLogMessage("✗ Error: Processor not initialized");
        return;
    }
    
    const auto sourceCode = getText();
    
    try
    {
        auto it = audioProcessor->compilers.find(currentLanguage.toStdString());
        if (it == audioProcessor->compilers.end())
        {
            addLogMessage("Error: Compiler not found for " + currentLanguage);
            return;
        }
        
        SampleShader outSample = nullptr;
        FrequencyShader outFrequency = nullptr;
        
        it->second->compileSource(sourceCode.toStdString(), outSample, outFrequency);
        
        if (outSample != nullptr && outFrequency != nullptr)
        {
            // Store compiled functions in processor's atomic pointers
            audioProcessor->activeSampleShader.store(outSample, std::memory_order_release);
            audioProcessor->activeFrequencyShader.store(outFrequency, std::memory_order_release);
            addLogMessage("✓ Compilation successful!");
        }
        else
        {
            addLogMessage("✗ Compilation failed - invalid output");
        }
    }
    catch (const std::exception& e)
    {
        addLogMessage("✗ Compilation error: " + juce::String(e.what()));
    }
}

//==============================================================================
void CodeEditor::addLogMessage(const juce::String& message)
{
    logMessages.push_back(message);
    
    // Keep log size reasonable
    if (logMessages.size() > 100)
        logMessages.erase(logMessages.begin());
    
    //logListBox.updateContent();
}

void CodeEditor::clearLog()
{
    logMessages.clear();
    //logListBox.updateContent();
}
