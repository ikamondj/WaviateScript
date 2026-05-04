/*
  ==============================================================================

    CodeEditorCompletion.h - C++ autocomplete provider for Waviate Shading Language.

    Provides symbol completion for WaviateSample, WaviateFrequency, WaviateCore APIs
    and C++ keywords extracted from the embedded prelude in ClangExternalCompiler.h.

  ==============================================================================
*/

#pragma once

#include <functional>
#include <vector>

#include <JuceHeader.h>

/**
 * Represents a single completion suggestion.
 */
struct CompletionItem
{
    enum class Kind
    {
        Function,
        Field,
        Keyword,
        Type,
        Constant,
        Class,
        Method,
        Variable
    };

    juce::String name;
    juce::String displayText;
    juce::String insertText;
    int cursorOffsetAfterInsert = 0;
    Kind kind = Kind::Function;
    juce::String documentation;
};

/**
 * Completion provider for Waviate Shading Language C++ code.
 */
class CompletionProvider
{
public:
    CompletionProvider();
    ~CompletionProvider() = default;

    std::vector<CompletionItem> getCompletions(const juce::String& sourceCode, int caretPos);
    std::vector<CompletionItem> getMemberCompletions(const juce::String& memberOwnerType);
    std::vector<CompletionItem> getGlobalCompletions(const juce::String& prefix = "");
    juce::String extractCompletionContext(const juce::String& sourceCode, int caretPos, juce::String& outMemberOwner);

private:
    void buildWaviateCoreMembers();
    void buildWaviateSampleMembers();
    void buildWaviateFrequencyMembers();
    void buildStructFields();
    void buildGlobalSymbols();

    static bool isIdentifierChar(juce::juce_wchar c);
    static std::vector<CompletionItem> filterByPrefix(const std::vector<CompletionItem>& items,
                                                       const juce::String& prefix);
    static std::vector<CompletionItem> sortByRelevance(const std::vector<CompletionItem>& items,
                                                        const juce::String& prefix);

    std::vector<CompletionItem> waviateCoreMembers;
    std::vector<CompletionItem> waviateSampleMembers;
    std::vector<CompletionItem> waviateFrequencyMembers;
    std::vector<CompletionItem> waviateSampleInputFields;
    std::vector<CompletionItem> waviateFrequencyInputFields;
    std::vector<CompletionItem> waviateComplexFields;
    std::vector<CompletionItem> globalFunctions;
    std::vector<CompletionItem> globalKeywords;
    std::vector<CompletionItem> globalTypes;
};

/**
 * Completion popup menu displayed in the editor.
 */
class CompletionPopupMenu : public juce::Component,
                            public juce::KeyListener
{
public:
    explicit CompletionPopupMenu(juce::CodeEditorComponent* editorComponent);
    ~CompletionPopupMenu() override;

    void showCompletions(const std::vector<CompletionItem>& items);
    void hideCompletions();
    bool isOpen() const { return isVisible() && ! completionItems.empty(); }
    bool acceptCompletion();

    std::function<void(const CompletionItem&)> onCompletionAccepted;

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

private:
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;

    juce::CodeEditorComponent* editor = nullptr;
    std::vector<CompletionItem> completionItems;
    int selectedIndex = 0;
    static constexpr int itemHeight = 20;
    static constexpr int maxVisibleItems = 10;
    static constexpr int menuWidth = 300;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompletionPopupMenu)
};
