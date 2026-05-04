/*
  ==============================================================================

    CodeEditorCompletion.cpp - Completion provider implementation.

  ==============================================================================
*/

#include "CodeEditorCompletion.h"
#include "WaviateCppLanguageModel.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

namespace
{
using waviate::language::FieldSymbol;
using waviate::language::FunctionSymbol;
using waviate::language::ParameterSymbol;
using waviate::language::SymbolKind;

CompletionItem::Kind toCompletionKind(SymbolKind kind)
{
    switch (kind)
    {
        case SymbolKind::Function: return CompletionItem::Kind::Function;
        case SymbolKind::Field: return CompletionItem::Kind::Field;
        case SymbolKind::Keyword: return CompletionItem::Kind::Keyword;
        case SymbolKind::Type: return CompletionItem::Kind::Type;
        case SymbolKind::Constant: return CompletionItem::Kind::Constant;
        case SymbolKind::Class: return CompletionItem::Kind::Class;
        case SymbolKind::Method: return CompletionItem::Kind::Method;
        case SymbolKind::Variable: return CompletionItem::Kind::Variable;
    }

    return CompletionItem::Kind::Function;
}

juce::String formatParameterList(const std::vector<ParameterSymbol>& parameters)
{
    juce::String text;

    for (size_t i = 0; i < parameters.size(); ++i)
    {
        const auto& parameter = parameters[i];
        if (i != 0)
            text << ", ";

        text << parameter.type << " " << parameter.name;
        if (! parameter.defaultValue.empty())
            text << " = " << parameter.defaultValue;
    }

    return text;
}

CompletionItem makeFunctionCompletion(const FunctionSymbol& symbol, bool insertCall)
{
    CompletionItem item;
    item.name = symbol.name;
    item.kind = toCompletionKind(symbol.kind);
    item.documentation = symbol.documentation;
    item.displayText = juce::String(symbol.name) + "(" + formatParameterList(symbol.parameters) + ")";

    if (! symbol.returnType.empty())
        item.displayText << " -> " << symbol.returnType;

    if (insertCall)
    {
        item.insertText = juce::String(symbol.name) + "()";
        item.cursorOffsetAfterInsert = symbol.parameters.empty() ? 0 : -1;
    }
    else
    {
        item.insertText = symbol.name;
        item.cursorOffsetAfterInsert = 0;
    }

    return item;
}

CompletionItem makeFieldCompletion(const FieldSymbol& symbol)
{
    CompletionItem item;
    item.name = symbol.name;
    item.insertText = symbol.name;
    item.cursorOffsetAfterInsert = 0;
    item.kind = toCompletionKind(symbol.kind);
    item.documentation = symbol.documentation;
    item.displayText = symbol.name;

    if (! symbol.type.empty())
        item.displayText << " : " << symbol.type;

    return item;
}

CompletionItem makeVariableCompletion(const juce::String& name, const juce::String& type)
{
    CompletionItem item;
    item.name = name;
    item.displayText = name + " : " + type;
    item.insertText = name;
    item.cursorOffsetAfterInsert = 0;
    item.kind = CompletionItem::Kind::Variable;
    item.documentation = "Visible local symbol.";
    return item;
}

template <typename Range, typename Converter>
void appendCompletions(std::vector<CompletionItem>& destination, const Range& source, Converter converter)
{
    for (const auto& symbol : source)
        destination.push_back(converter(symbol));
}

bool hasCompletionNamed(const std::vector<CompletionItem>& items, const juce::String& name)
{
    return std::any_of(items.begin(), items.end(), [&name](const CompletionItem& item) {
        return item.name == name;
    });
}

int kindRank(CompletionItem::Kind kind)
{
    switch (kind)
    {
        case CompletionItem::Kind::Variable: return 0;
        case CompletionItem::Kind::Field: return 1;
        case CompletionItem::Kind::Method: return 2;
        case CompletionItem::Kind::Function: return 3;
        case CompletionItem::Kind::Class: return 4;
        case CompletionItem::Kind::Type: return 5;
        case CompletionItem::Kind::Constant: return 6;
        case CompletionItem::Kind::Keyword: return 7;
    }

    return 8;
}

juce::String trimExpression(juce::String expression)
{
    expression = expression.trim();

    while (expression.startsWith("(") && expression.endsWith(")") && expression.length() > 2)
        expression = expression.substring(1, expression.length() - 1).trim();

    return expression;
}

bool isKnownWaviateObjectType(const juce::String& type)
{
    return type == "WaviateCore"
        || type == "WaviateSample"
        || type == "WaviateFrequency"
        || type == "WaviateSampleInput"
        || type == "WaviateFrequencyInput"
        || type == "WaviateComplex";
}

bool isIdentifierCharLocal(juce::juce_wchar c)
{
    return juce::CharacterFunctions::isLetterOrDigit(c) || c == '_';
}

juce::String extractOwnerExpressionEndingAt(const juce::String& sourceCode, int ownerEnd)
{
    while (ownerEnd >= 0 && juce::CharacterFunctions::isWhitespace(sourceCode[ownerEnd]))
        --ownerEnd;

    if (ownerEnd < 0)
        return {};

    if (sourceCode[ownerEnd] == ')')
    {
        int depth = 0;
        int openParen = -1;

        for (int i = ownerEnd; i >= 0; --i)
        {
            if (sourceCode[i] == ')')
                ++depth;
            else if (sourceCode[i] == '(')
            {
                --depth;
                if (depth == 0)
                {
                    openParen = i;
                    break;
                }
            }
        }

        if (openParen < 0)
            return {};

        int nameEnd = openParen - 1;
        while (nameEnd >= 0 && juce::CharacterFunctions::isWhitespace(sourceCode[nameEnd]))
            --nameEnd;

        if (nameEnd < 0 || ! isIdentifierCharLocal(sourceCode[nameEnd]))
            return {};

        int nameStart = nameEnd;
        while (nameStart > 0 && isIdentifierCharLocal(sourceCode[nameStart - 1]))
            --nameStart;

        int expressionStart = nameStart;
        int accessEnd = nameStart - 1;
        while (accessEnd >= 0 && juce::CharacterFunctions::isWhitespace(sourceCode[accessEnd]))
            --accessEnd;

        const bool hasDot = accessEnd >= 0 && sourceCode[accessEnd] == '.';
        const bool hasArrow = accessEnd >= 1 && sourceCode[accessEnd - 1] == '-' && sourceCode[accessEnd] == '>';

        if (hasDot || hasArrow)
        {
            int leftEnd = hasDot ? accessEnd - 1 : accessEnd - 2;
            while (leftEnd >= 0 && juce::CharacterFunctions::isWhitespace(sourceCode[leftEnd]))
                --leftEnd;

            if (leftEnd >= 0 && isIdentifierCharLocal(sourceCode[leftEnd]))
            {
                int leftStart = leftEnd;
                while (leftStart > 0 && isIdentifierCharLocal(sourceCode[leftStart - 1]))
                    --leftStart;

                expressionStart = leftStart;
            }
        }

        return sourceCode.substring(expressionStart, ownerEnd + 1).trim();
    }

    if (! isIdentifierCharLocal(sourceCode[ownerEnd]))
        return {};

    int ownerStart = ownerEnd;
    while (ownerStart > 0 && isIdentifierCharLocal(sourceCode[ownerStart - 1]))
        --ownerStart;

    return sourceCode.substring(ownerStart, ownerEnd + 1).trim();
}
} // namespace

//==============================================================================
// CompletionProvider
//==============================================================================

CompletionProvider::CompletionProvider() = default;

std::vector<CompletionItem> CompletionProvider::getCompletions(const juce::String& sourceCode, int caretPos)
{
    if (! isCodePosition(sourceCode, caretPos))
        return {};

    auto context = getCompletionContext(sourceCode, caretPos);
    const auto symbols = buildVisibleSymbolTable(sourceCode, caretPos);

    if (context.isMemberAccess)
    {
        context.memberOwnerType = resolveExpressionType(context.memberOwnerExpression, symbols);
        if (context.memberOwnerType.isEmpty())
            return {};

        auto completions = getMemberCompletionsForType(context.memberOwnerType);
        return sortByRelevance(filterByPrefix(completions, context.prefix), context.prefix);
    }

    return getGlobalCompletions(context.prefix, symbols);
}

std::vector<CompletionItem> CompletionProvider::getMemberCompletions(const juce::String& memberOwnerType)
{
    return getMemberCompletionsForType(memberOwnerType);
}

std::vector<CompletionItem> CompletionProvider::getGlobalCompletions(const juce::String& prefix)
{
    return getGlobalCompletions(prefix, {});
}

juce::String CompletionProvider::extractCompletionContext(const juce::String& sourceCode,
                                                          int caretPos,
                                                          juce::String& outMemberOwner)
{
    const auto context = getCompletionContext(sourceCode, caretPos);
    outMemberOwner = context.memberOwnerExpression;
    return context.prefix;
}

CompletionProvider::CompletionContext CompletionProvider::getCompletionContext(const juce::String& sourceCode,
                                                                               int caretPos) const
{
    CompletionContext context;

    if (sourceCode.isEmpty())
        return context;

    caretPos = juce::jlimit(0, sourceCode.length(), caretPos);

    int prefixStart = caretPos;
    while (prefixStart > 0 && isIdentifierChar(sourceCode[prefixStart - 1]))
        --prefixStart;

    context.prefix = sourceCode.substring(prefixStart, caretPos);

    int operatorEnd = prefixStart - 1;
    while (operatorEnd >= 0 && juce::CharacterFunctions::isWhitespace(sourceCode[operatorEnd]))
        --operatorEnd;

    const bool isDotAccess = operatorEnd >= 0 && sourceCode[operatorEnd] == '.';
    const bool isArrowAccess = operatorEnd >= 1 && sourceCode[operatorEnd - 1] == '-' && sourceCode[operatorEnd] == '>';

    if (! isDotAccess && ! isArrowAccess)
        return context;

    const int ownerEnd = isDotAccess ? operatorEnd - 1 : operatorEnd - 2;
    context.memberOwnerExpression = extractOwnerExpressionEndingAt(sourceCode, ownerEnd);
    context.isMemberAccess = context.memberOwnerExpression.isNotEmpty();
    return context;
}

CompletionProvider::SymbolTable CompletionProvider::buildVisibleSymbolTable(const juce::String& sourceCode,
                                                                            int caretPos) const
{
    SymbolTable symbols;

    caretPos = juce::jlimit(0, sourceCode.length(), caretPos);
    const auto stripped = stripCommentsAndStrings(sourceCode.substring(0, caretPos)).toStdString();

    const std::regex typedDeclaration(
        R"((?:^|[;{}\n,(])\s*((?:(?:const|volatile)\s+)*(?:bool|char|float|double|int|int32_t|uint8_t|uint32_t|uint64_t|WaviateSample|WaviateFrequency|WaviateCore|WaviateSampleInput|WaviateSampleStateWriter|WaviateFrequencyInput|WaviateFrequencyStateWriter|WaviateComplex)\s*(?:(?:const|volatile)\s*)?[*&]?)\s+([A-Za-z_][A-Za-z0-9_]*))");

    for (auto it = std::sregex_iterator(stripped.begin(), stripped.end(), typedDeclaration);
         it != std::sregex_iterator();
         ++it)
    {
        const auto match = *it;
        const auto type = normaliseTypeName(juce::String(match[1].str()));
        const auto name = juce::String(match[2].str());

        if (type.isNotEmpty())
            symbols[name] = type;
    }

    const std::regex autoConstruction(
        R"(\bauto\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(WaviateSample|WaviateFrequency|WaviateCore|WaviateComplex)\b)");

    for (auto it = std::sregex_iterator(stripped.begin(), stripped.end(), autoConstruction);
         it != std::sregex_iterator();
         ++it)
    {
        const auto match = *it;
        symbols[juce::String(match[1].str())] = juce::String(match[2].str());
    }

    const std::regex autoMemberCall(
        R"(\bauto\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([A-Za-z_][A-Za-z0-9_]*)\s*(?:\.|->)\s*([A-Za-z_][A-Za-z0-9_]*)\s*\()");

    for (auto it = std::sregex_iterator(stripped.begin(), stripped.end(), autoMemberCall);
         it != std::sregex_iterator();
         ++it)
    {
        const auto match = *it;
        const auto variableName = juce::String(match[1].str());
        const auto ownerName = juce::String(match[2].str());
        const auto memberName = juce::String(match[3].str());

        auto owner = symbols.find(ownerName);
        if (owner == symbols.end())
            continue;

        const auto returnType = resolveMemberReturnType(owner->second, memberName);
        if (returnType.isNotEmpty())
            symbols[variableName] = returnType;
    }

    return symbols;
}

juce::String CompletionProvider::resolveExpressionType(const juce::String& expression,
                                                       const SymbolTable& symbols) const
{
    const auto trimmedExpression = trimExpression(expression);
    if (trimmedExpression.isEmpty())
        return {};

    const auto directType = normaliseTypeName(trimmedExpression);
    if (isKnownWaviateObjectType(directType))
        return directType;

    auto symbol = symbols.find(trimmedExpression);
    if (symbol != symbols.end())
        return symbol->second;

    const std::regex memberCall(
        R"(^\s*([A-Za-z_][A-Za-z0-9_]*)\s*(?:\.|->)\s*([A-Za-z_][A-Za-z0-9_]*)\s*\([^)]*\)\s*$)");
    std::smatch match;
    const auto expressionText = trimmedExpression.toStdString();

    if (std::regex_match(expressionText, match, memberCall))
    {
        const auto ownerName = juce::String(match[1].str());
        const auto memberName = juce::String(match[2].str());
        const auto ownerType = resolveExpressionType(ownerName, symbols);

        if (ownerType.isNotEmpty())
            return resolveMemberReturnType(ownerType, memberName);
    }

    return {};
}

juce::String CompletionProvider::resolveMemberReturnType(const juce::String& ownerType,
                                                         const juce::String& memberName) const
{
    const auto normalisedOwnerType = normaliseTypeName(ownerType);

    auto findReturnType = [&memberName](const std::vector<FunctionSymbol>& functions) -> juce::String {
        const auto found = std::find_if(functions.begin(), functions.end(), [&memberName](const FunctionSymbol& function) {
            return function.name == memberName;
        });

        if (found == functions.end())
            return {};

        return normaliseTypeName(juce::String(found->returnType));
    };

    if (normalisedOwnerType == "WaviateSample")
    {
        if (auto returnType = findReturnType(waviate::language::waviateSampleMemberFunctions()); returnType.isNotEmpty())
            return returnType;
        return findReturnType(waviate::language::waviateCoreMemberFunctions());
    }

    if (normalisedOwnerType == "WaviateFrequency")
    {
        if (auto returnType = findReturnType(waviate::language::waviateFrequencyMemberFunctions()); returnType.isNotEmpty())
            return returnType;
        return findReturnType(waviate::language::waviateCoreMemberFunctions());
    }

    if (normalisedOwnerType == "WaviateCore")
        return findReturnType(waviate::language::waviateCoreMemberFunctions());

    return {};
}

std::vector<CompletionItem> CompletionProvider::getGlobalCompletions(const juce::String& prefix,
                                                                     const SymbolTable& symbols) const
{
    std::vector<CompletionItem> result;

    appendCompletions(result, waviate::language::waviateEntryPoints(), [](const FunctionSymbol& symbol) {
        return makeFunctionCompletion(symbol, false);
    });

    appendCompletions(result, waviate::language::cppBuiltinTypes(), makeFieldCompletion);
    appendCompletions(result, waviate::language::cppKeywords(), makeFieldCompletion);

    for (const auto& [name, type] : symbols)
    {
        if (! hasCompletionNamed(result, name))
            result.push_back(makeVariableCompletion(name, type));
    }

    return sortByRelevance(filterByPrefix(result, prefix), prefix);
}

std::vector<CompletionItem> CompletionProvider::getMemberCompletionsForType(const juce::String& memberOwnerType) const
{
    const auto type = normaliseTypeName(memberOwnerType);
    std::vector<CompletionItem> result;

    if (type == "WaviateSample" || type == "WaviateFrequency" || type == "WaviateCore")
    {
        appendCompletions(result, waviate::language::waviateCoreMemberFunctions(), [](const FunctionSymbol& symbol) {
            return makeFunctionCompletion(symbol, true);
        });
    }

    if (type == "WaviateSample")
    {
        appendCompletions(result, waviate::language::waviateSampleMemberFunctions(), [](const FunctionSymbol& symbol) {
            return makeFunctionCompletion(symbol, true);
        });
    }

    if (type == "WaviateFrequency")
    {
        appendCompletions(result, waviate::language::waviateFrequencyMemberFunctions(), [](const FunctionSymbol& symbol) {
            return makeFunctionCompletion(symbol, true);
        });
    }

    if (type == "WaviateSampleInput")
        appendCompletions(result, waviate::language::waviateSampleInputFields(), makeFieldCompletion);

    if (type == "WaviateFrequencyInput")
        appendCompletions(result, waviate::language::waviateFrequencyInputFields(), makeFieldCompletion);

    if (type == "WaviateComplex")
        appendCompletions(result, waviate::language::waviateComplexFields(), makeFieldCompletion);

    return sortByRelevance(result, {});
}

bool CompletionProvider::isIdentifierChar(juce::juce_wchar c)
{
    return juce::CharacterFunctions::isLetterOrDigit(c) || c == '_';
}

bool CompletionProvider::isCodePosition(const juce::String& sourceCode, int caretPos)
{
    enum class ScanState
    {
        normal,
        lineComment,
        blockComment,
        stringLiteral,
        charLiteral
    };

    const auto text = sourceCode.toStdString();
    const auto length = static_cast<int>(text.size());
    caretPos = juce::jlimit(0, length, caretPos);
    auto state = ScanState::normal;

    for (int i = 0; i < caretPos; ++i)
    {
        const char c = text[static_cast<size_t>(i)];
        const char next = (i + 1 < length) ? text[static_cast<size_t>(i + 1)] : '\0';

        switch (state)
        {
            case ScanState::normal:
                if (c == '/' && next == '/')
                {
                    ++i;
                    state = ScanState::lineComment;
                }
                else if (c == '/' && next == '*')
                {
                    ++i;
                    state = ScanState::blockComment;
                }
                else if (c == '"')
                {
                    state = ScanState::stringLiteral;
                }
                else if (c == '\'')
                {
                    state = ScanState::charLiteral;
                }
                break;

            case ScanState::lineComment:
                if (c == '\n' || c == '\r')
                    state = ScanState::normal;
                break;

            case ScanState::blockComment:
                if (c == '*' && next == '/')
                {
                    ++i;
                    state = ScanState::normal;
                }
                break;

            case ScanState::stringLiteral:
                if (c == '\\' && next != '\0')
                    ++i;
                else if (c == '"')
                    state = ScanState::normal;
                break;

            case ScanState::charLiteral:
                if (c == '\\' && next != '\0')
                    ++i;
                else if (c == '\'')
                    state = ScanState::normal;
                break;
        }
    }

    return state == ScanState::normal;
}

juce::String CompletionProvider::stripCommentsAndStrings(const juce::String& sourceCode)
{
    enum class ScanState
    {
        normal,
        lineComment,
        blockComment,
        stringLiteral,
        charLiteral
    };

    auto text = sourceCode.toStdString();
    auto stripped = text;
    auto state = ScanState::normal;

    for (size_t i = 0; i < text.size(); ++i)
    {
        const char c = text[i];
        const char next = (i + 1 < text.size()) ? text[i + 1] : '\0';

        switch (state)
        {
            case ScanState::normal:
                if (c == '/' && next == '/')
                {
                    stripped[i] = ' ';
                    stripped[i + 1] = ' ';
                    ++i;
                    state = ScanState::lineComment;
                }
                else if (c == '/' && next == '*')
                {
                    stripped[i] = ' ';
                    stripped[i + 1] = ' ';
                    ++i;
                    state = ScanState::blockComment;
                }
                else if (c == '"')
                {
                    stripped[i] = ' ';
                    state = ScanState::stringLiteral;
                }
                else if (c == '\'')
                {
                    stripped[i] = ' ';
                    state = ScanState::charLiteral;
                }
                break;

            case ScanState::lineComment:
                if (c == '\n' || c == '\r')
                    state = ScanState::normal;
                else
                    stripped[i] = ' ';
                break;

            case ScanState::blockComment:
                stripped[i] = (c == '\n' || c == '\r') ? c : ' ';
                if (c == '*' && next == '/')
                {
                    stripped[i + 1] = ' ';
                    ++i;
                    state = ScanState::normal;
                }
                break;

            case ScanState::stringLiteral:
                stripped[i] = (c == '\n' || c == '\r') ? c : ' ';
                if (c == '\\' && next != '\0')
                {
                    stripped[i + 1] = (next == '\n' || next == '\r') ? next : ' ';
                    ++i;
                }
                else if (c == '"')
                {
                    state = ScanState::normal;
                }
                break;

            case ScanState::charLiteral:
                stripped[i] = (c == '\n' || c == '\r') ? c : ' ';
                if (c == '\\' && next != '\0')
                {
                    stripped[i + 1] = (next == '\n' || next == '\r') ? next : ' ';
                    ++i;
                }
                else if (c == '\'')
                {
                    state = ScanState::normal;
                }
                break;
        }
    }

    return stripped;
}

juce::String CompletionProvider::normaliseTypeName(juce::String typeName)
{
    auto text = typeName.toStdString();
    for (auto& character : text)
    {
        if (character == '*' || character == '&')
            character = ' ';
    }

    std::istringstream stream(text);
    std::string token;
    std::string lastTypeToken;

    while (stream >> token)
    {
        if (token == "const" || token == "volatile" || token == "static" || token == "extern")
            continue;

        lastTypeToken = token;
    }

    return lastTypeToken;
}

std::vector<CompletionItem> CompletionProvider::filterByPrefix(const std::vector<CompletionItem>& items,
                                                               const juce::String& prefix)
{
    if (prefix.isEmpty())
        return items;

    std::vector<CompletionItem> result;

    for (const auto& item : items)
    {
        if (item.name.startsWithIgnoreCase(prefix))
            result.push_back(item);
    }

    return result;
}

std::vector<CompletionItem> CompletionProvider::sortByRelevance(const std::vector<CompletionItem>& items,
                                                                const juce::String& prefix)
{
    auto result = items;

    std::sort(result.begin(), result.end(), [&prefix](const CompletionItem& a, const CompletionItem& b) {
        const bool aExact = prefix.isNotEmpty() && a.name.equalsIgnoreCase(prefix);
        const bool bExact = prefix.isNotEmpty() && b.name.equalsIgnoreCase(prefix);
        if (aExact != bExact)
            return aExact;

        const auto aKindRank = kindRank(a.kind);
        const auto bKindRank = kindRank(b.kind);
        if (aKindRank != bKindRank)
            return aKindRank < bKindRank;

        if (a.name.length() != b.name.length())
            return a.name.length() < b.name.length();

        return a.name.compareIgnoreCase(b.name) < 0;
    });

    return result;
}

//==============================================================================
// CompletionPopupMenu
//==============================================================================

CompletionPopupMenu::CompletionPopupMenu(juce::CodeEditorComponent* editorComponent)
    : editor(editorComponent)
{
    setInterceptsMouseClicks(true, true);
    addKeyListener(this);
}

CompletionPopupMenu::~CompletionPopupMenu()
{
    removeKeyListener(this);
}

void CompletionPopupMenu::showCompletions(const std::vector<CompletionItem>& items)
{
    completionItems = items;
    selectedIndex = 0;
    firstVisibleIndex = 0;

    if (editor == nullptr || completionItems.empty())
    {
        hideCompletions();
        return;
    }

    const auto caretBounds = editor->getCharacterBounds(editor->getCaretPos());
    const auto popupPos = getParentComponent() != nullptr
        ? getParentComponent()->getLocalPoint(editor, caretBounds.getBottomLeft())
        : caretBounds.getBottomLeft();
    const auto popupHeight = juce::jmin(static_cast<int>(completionItems.size()), maxVisibleItems) * itemHeight;
    auto popupBounds = juce::Rectangle<int>(popupPos.x, popupPos.y, menuWidth, popupHeight);

    if (auto* parent = getParentComponent())
        popupBounds = popupBounds.constrainedWithin(parent->getLocalBounds());

    setBounds(popupBounds);

    setVisible(true);
    toFront(false);
    repaint();
}

void CompletionPopupMenu::hideCompletions()
{
    completionItems.clear();
    selectedIndex = 0;
    firstVisibleIndex = 0;
    setVisible(false);
}

bool CompletionPopupMenu::acceptCompletion()
{
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(completionItems.size()))
        return false;

    if (onCompletionAccepted != nullptr)
        onCompletionAccepted(completionItems[static_cast<size_t>(selectedIndex)]);

    return true;
}

void CompletionPopupMenu::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff25272b));

    const auto visibleCount = juce::jmin(static_cast<int>(completionItems.size()) - firstVisibleIndex,
                                        maxVisibleItems);

    for (int row = 0; row < visibleCount; ++row)
    {
        const auto itemIndex = firstVisibleIndex + row;
        auto rect = juce::Rectangle<int>(0, row * itemHeight, getWidth(), itemHeight);

        if (itemIndex == selectedIndex)
        {
            g.setColour(juce::Colour(0xff0e639c));
            g.fillRect(rect);
        }

        auto badgeArea = rect.removeFromLeft(kindBadgeWidth).reduced(5, 4);
        g.setColour(getKindColour(completionItems[static_cast<size_t>(itemIndex)].kind));
        g.fillRect(badgeArea);

        g.setColour(juce::Colours::white);
        g.setFont(10.0f);
        g.drawText(getKindLabel(completionItems[static_cast<size_t>(itemIndex)].kind),
                   badgeArea,
                   juce::Justification::centred,
                   true);

        g.setFont(13.0f);
        g.setColour(juce::Colours::white);
        g.drawText(completionItems[static_cast<size_t>(itemIndex)].displayText,
                   rect.reduced(4, 2),
                   juce::Justification::centredLeft,
                   true);
    }

    g.setColour(juce::Colour(0xff4b4d52));
    g.drawRect(getLocalBounds(), 1);
}

void CompletionPopupMenu::resized()
{
}

bool CompletionPopupMenu::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    auto keepSelectionVisible = [this] {
        if (selectedIndex < firstVisibleIndex)
            firstVisibleIndex = selectedIndex;
        else if (selectedIndex >= firstVisibleIndex + maxVisibleItems)
            firstVisibleIndex = selectedIndex - maxVisibleItems + 1;
    };

    if (key.getKeyCode() == juce::KeyPress::upKey)
    {
        selectedIndex = juce::jmax(0, selectedIndex - 1);
        keepSelectionVisible();
        repaint();
        return true;
    }

    if (key.getKeyCode() == juce::KeyPress::downKey)
    {
        selectedIndex = juce::jmin(static_cast<int>(completionItems.size()) - 1, selectedIndex + 1);
        keepSelectionVisible();
        repaint();
        return true;
    }

    if (key.getKeyCode() == juce::KeyPress::returnKey || key.getKeyCode() == juce::KeyPress::tabKey)
        return acceptCompletion();

    if (key.getKeyCode() == juce::KeyPress::escapeKey)
    {
        hideCompletions();
        return true;
    }

    return false;
}

void CompletionPopupMenu::mouseUp(const juce::MouseEvent& e)
{
    const int itemIndex = firstVisibleIndex + (e.y / itemHeight);
    if (itemIndex >= 0 && itemIndex < static_cast<int>(completionItems.size()))
    {
        selectedIndex = itemIndex;
        acceptCompletion();
    }
}

void CompletionPopupMenu::mouseMove(const juce::MouseEvent& e)
{
    const int itemIndex = firstVisibleIndex + (e.y / itemHeight);
    if (itemIndex >= 0 && itemIndex < static_cast<int>(completionItems.size()))
    {
        selectedIndex = itemIndex;
        repaint();
    }
}

juce::String CompletionPopupMenu::getKindLabel(CompletionItem::Kind kind)
{
    switch (kind)
    {
        case CompletionItem::Kind::Function: return "fn";
        case CompletionItem::Kind::Field: return "fld";
        case CompletionItem::Kind::Keyword: return "kw";
        case CompletionItem::Kind::Type: return "type";
        case CompletionItem::Kind::Constant: return "const";
        case CompletionItem::Kind::Class: return "cls";
        case CompletionItem::Kind::Method: return "meth";
        case CompletionItem::Kind::Variable: return "var";
    }

    return "?";
}

juce::Colour CompletionPopupMenu::getKindColour(CompletionItem::Kind kind)
{
    switch (kind)
    {
        case CompletionItem::Kind::Function: return juce::Colour(0xff8a63d2);
        case CompletionItem::Kind::Method: return juce::Colour(0xff3c8dbc);
        case CompletionItem::Kind::Field: return juce::Colour(0xff5d9b61);
        case CompletionItem::Kind::Variable: return juce::Colour(0xffb8792c);
        case CompletionItem::Kind::Class: return juce::Colour(0xffb85c85);
        case CompletionItem::Kind::Type: return juce::Colour(0xff6b8fbc);
        case CompletionItem::Kind::Keyword: return juce::Colour(0xff666a70);
        case CompletionItem::Kind::Constant: return juce::Colour(0xff8f7a3d);
    }

    return juce::Colour(0xff666a70);
}
