/*
  ==============================================================================

    CodeEditorCompletion.h - C++ autocomplete provider for Waviate Shading Language.

    Provides symbol completion for WaviateSample, WaviateFrequency, WaviateCore APIs
    and C++ keywords extracted from the embedded prelude in ClangExternalCompiler.h.

  ==============================================================================
*/

#pragma once

#include <functional>
#include <map>
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
    // Offset from the end of insertText after insertion. Negative values place
    // the caret inside call parentheses.
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
    struct CompletionContext
    {
        juce::String prefix;
        juce::String memberOwnerExpression;
        juce::String memberOwnerType;
        bool isMemberAccess = false;
    };

    using SymbolTable = std::map<juce::String, juce::String>;

    CompletionContext getCompletionContext(const juce::String& sourceCode, int caretPos) const;
    SymbolTable buildVisibleSymbolTable(const juce::String& sourceCode, int caretPos) const;
    juce::String resolveExpressionType(const juce::String& expression, const SymbolTable& symbols) const;
    juce::String resolveMemberReturnType(const juce::String& ownerType, const juce::String& memberName) const;
    std::vector<CompletionItem> getGlobalCompletions(const juce::String& prefix, const SymbolTable& symbols) const;
    std::vector<CompletionItem> getMemberCompletionsForType(const juce::String& memberOwnerType) const;

    static bool isIdentifierChar(juce::juce_wchar c);
    static bool isCodePosition(const juce::String& sourceCode, int caretPos);
    static juce::String stripCommentsAndStrings(const juce::String& sourceCode);
    static juce::String normaliseTypeName(juce::String typeName);
    static std::vector<CompletionItem> filterByPrefix(const std::vector<CompletionItem>& items,
                                                       const juce::String& prefix);
    static std::vector<CompletionItem> sortByRelevance(const std::vector<CompletionItem>& items,
                                                        const juce::String& prefix);
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
    static juce::String getKindLabel(CompletionItem::Kind kind);
    static juce::Colour getKindColour(CompletionItem::Kind kind);

    juce::CodeEditorComponent* editor = nullptr;
    std::vector<CompletionItem> completionItems;
    int selectedIndex = 0;
    int firstVisibleIndex = 0;
    static constexpr int itemHeight = 22;
    static constexpr int maxVisibleItems = 10;
    static constexpr int menuWidth = 420;
    static constexpr int kindBadgeWidth = 44;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompletionPopupMenu)
};
