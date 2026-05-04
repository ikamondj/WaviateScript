/*
  ==============================================================================

    CodeEditorCompletion.h - Autocomplete provider for Waviate Shading Language

  ==============================================================================
*/

#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <vector>
#include <functional>

//==============================================================================
// CompletionItem - Represents a single completion suggestion
//==============================================================================
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

    juce::String name;              // Symbol identifier
    juce::String displayText;       // What shows in UI dropdown
    juce::String insertText;        // Text to insert (may include ${1:...} for cursor position)
    int cursorOffsetAfterInsert = 0; // How much to move cursor after insertion
    Kind kind = Kind::Function;     // Type of completion
    juce::String documentation;     // Tooltip/help text
};

//==============================================================================
// CompletionProvider - Symbol database and filtering logic
//==============================================================================
class CompletionProvider
{
public:
    CompletionProvider();
    ~CompletionProvider() = default;

    // Main entry point: get completions at a specific position
    std::vector<CompletionItem> getCompletions(const juce::String& sourceCode, int caretPos);

    // Get completions for member access (e.g., wav.method())
    std::vector<CompletionItem> getMemberCompletions(const juce::String& memberOwnerType);

    // Get global completions (keywords, types, functions)
    std::vector<CompletionItem> getGlobalCompletions(const juce::String& prefix = "");

    // Extract completion context from source code
    // Returns prefix (what's being typed), updates memberOwner if accessing a member
    juce::String extractCompletionContext(const juce::String& sourceCode, int caretPos, juce::String& memberOwner);

private:
    // Symbol database builders
    void buildWaviateCoreMembers();
    void buildWaviateSampleMembers();
    void buildWaviateFrequencyMembers();
    void buildStructFields();
    void buildGlobalSymbols();

    // Helper methods
    std::vector<CompletionItem> filterByPrefix(const std::vector<CompletionItem>& items, const juce::String& prefix);
    std::vector<CompletionItem> sortByRelevance(const std::vector<CompletionItem>& items, const juce::String& prefix);

    // Symbol collections
    std::vector<CompletionItem> waviateCoreMembers;
    std::vector<CompletionItem> waviateSampleMembers;
    std::vector<CompletionItem> waviateFrequencyMembers;
    std::vector<CompletionItem> waviateInputMembers;
    std::vector<CompletionItem> waviateComplexMembers;
    std::vector<CompletionItem> globalTypes;
    std::vector<CompletionItem> globalKeywords;
    std::vector<CompletionItem> globalFunctions;
};

//==============================================================================
// CompletionPopupMenu - UI component for displaying suggestions
//==============================================================================
class CompletionPopupMenu : public juce::Component, public juce::KeyListener
{
public:
    explicit CompletionPopupMenu(juce::CodeEditorComponent* editor);
    ~CompletionPopupMenu() override = default;

    // Show completions at editor caret position
    void showCompletions(const std::vector<CompletionItem>& items, juce::CodeEditorComponent& editor, int caretPos);

    // Hide the popup
    void hideCompletions();

    // Accept current selection
    void acceptCompletion();

    // Callback when completion is selected
    std::function<void(const CompletionItem&)> onCompletionAccepted;

    // Component methods
    void paint(juce::Graphics& g) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

    // KeyListener methods
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

private:
    juce::CodeEditorComponent* editorComponent;
    std::vector<CompletionItem> completionItems;
    int selectedIndex = 0;
    int maxVisibleItems = 10;

    void updateSelection(int newIndex);
    int getItemAtY(int y) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompletionPopupMenu)
};
