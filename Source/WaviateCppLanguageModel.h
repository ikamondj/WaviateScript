/*
  ==============================================================================

    WaviateCppLanguageModel.h - Shared C++ shader API metadata.

  ==============================================================================
*/

#pragma once

#include <string>
#include <vector>

namespace waviate::language
{
enum class SymbolKind
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

struct ParameterSymbol
{
    std::string type;
    std::string name;
    std::string defaultValue;
};

struct FunctionSymbol
{
    std::string name;
    std::string returnType;
    std::vector<ParameterSymbol> parameters;
    SymbolKind kind = SymbolKind::Function;
    std::string documentation;
};

struct FieldSymbol
{
    std::string name;
    std::string type;
    SymbolKind kind = SymbolKind::Field;
    std::string documentation;
};

const std::vector<FunctionSymbol>& waviateCoreMemberFunctions();
const std::vector<FunctionSymbol>& waviateSampleMemberFunctions();
const std::vector<FunctionSymbol>& waviateFrequencyMemberFunctions();
const std::vector<FieldSymbol>& waviateSampleInputFields();
const std::vector<FieldSymbol>& waviateFrequencyInputFields();
const std::vector<FieldSymbol>& waviateComplexFields();
const std::vector<FieldSymbol>& cppBuiltinTypes();
const std::vector<FieldSymbol>& cppKeywords();
const std::vector<FunctionSymbol>& waviateEntryPoints();

bool isKnownWaviateType(const std::string& typeName);
std::string buildEmbeddedCppApi();
} // namespace waviate::language
