#include "WaviateShaderSerializer.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

namespace
{
constexpr const char* waviateShaderSchema = "com.waviate.shader";
constexpr int waviateShaderSchemaVersion = 1;

bool isIdentifierChar (const char c) noexcept
{
    const auto value = static_cast<unsigned char> (c);
    return std::isalnum (value) != 0 || c == '_';
}

bool isDigit (const char c) noexcept
{
    return std::isdigit (static_cast<unsigned char> (c)) != 0;
}

bool isWhitespace (const char c) noexcept
{
    return std::isspace (static_cast<unsigned char> (c)) != 0;
}

std::string trimRight (std::string text)
{
    while (! text.empty() && isWhitespace (text.back()))
        text.pop_back();

    return text;
}

bool endsWithEscapedNewline (const std::string& text)
{
    auto i = text.size();

    while (i > 0 && (text[i - 1] == '\r' || text[i - 1] == '\n'))
        --i;

    while (i > 0 && (text[i - 1] == ' ' || text[i - 1] == '\t'))
        --i;

    return i > 0 && text[i - 1] == '\\';
}

size_t readPreprocessorDirective (const std::string& input, const size_t start)
{
    auto i = start;

    while (i < input.size())
    {
        const auto lineStart = i;

        while (i < input.size() && input[i] != '\n')
            ++i;

        if (i < input.size())
            ++i;

        const auto line = input.substr (lineStart, i - lineStart);

        if (! endsWithEscapedNewline (line))
            break;
    }

    return i;
}

bool matchAt (const std::string& input, const size_t index, const char* token)
{
    const auto length = std::strlen (token);
    return index + length <= input.size() && input.compare (index, length, token) == 0;
}

size_t rawStringPrefixLength (const std::string& input, const size_t index)
{
    for (const auto* prefix : { "u8R\"", "LR\"", "uR\"", "UR\"", "R\"" })
        if (matchAt (input, index, prefix))
            return std::strlen (prefix);

    return 0;
}

size_t quotedStringPrefixLength (const std::string& input, const size_t index, const char quote)
{
    const auto suffix = std::string (1, quote);

    for (const auto* prefix : { "u8", "L", "u", "U", "" })
    {
        const auto candidate = std::string (prefix) + suffix;

        if (index + candidate.size() <= input.size() && input.compare (index, candidate.size(), candidate) == 0)
            return candidate.size();
    }

    return 0;
}

size_t readRawStringLiteral (const std::string& input, const size_t start, const size_t prefixLength)
{
    const auto delimiterStart = start + prefixLength;
    auto openParen = delimiterStart;

    while (openParen < input.size() && input[openParen] != '(')
        ++openParen;

    if (openParen >= input.size())
        return input.size();

    const auto delimiter = input.substr (delimiterStart, openParen - delimiterStart);
    const auto terminator = ")" + delimiter + "\"";
    const auto end = input.find (terminator, openParen + 1);

    return end == std::string::npos ? input.size() : end + terminator.size();
}

size_t readQuotedLiteral (const std::string& input, const size_t start, const size_t prefixLength)
{
    const auto quote = input[start + prefixLength - 1];
    auto i = start + prefixLength;
    bool escaped = false;

    while (i < input.size())
    {
        const auto c = input[i++];

        if (escaped)
        {
            escaped = false;
            continue;
        }

        if (c == '\\')
        {
            escaped = true;
            continue;
        }

        if (c == quote)
            break;
    }

    return i;
}

size_t readStringLiteral (const std::string& input, const size_t start)
{
    if (const auto prefixLength = rawStringPrefixLength (input, start))
        return readRawStringLiteral (input, start, prefixLength);

    if (const auto prefixLength = quotedStringPrefixLength (input, start, '"'))
        return readQuotedLiteral (input, start, prefixLength);

    if (const auto prefixLength = quotedStringPrefixLength (input, start, '\''))
        return readQuotedLiteral (input, start, prefixLength);

    return start;
}

std::string readPunctuator (const std::string& input, const size_t start)
{
    static constexpr const char* punctuators[] =
    {
        "<<=", ">>=", "...", "->*", "::", "++", "--", "->", "&&", "||",
        "==", "!=", "<=", ">=", "+=", "-=", "*=", "/=", "%=", "&=", "|=",
        "^=", "<<", ">>", "##"
    };

    for (const auto* punctuator : punctuators)
        if (matchAt (input, start, punctuator))
            return punctuator;

    return input.substr (start, 1);
}

bool needsSpaceBetween (const std::string& previous, const std::string& current)
{
    if (previous.empty() || current.empty())
        return false;

    const auto prev = previous.back();
    const auto next = current.front();

    if (isIdentifierChar (prev) && isIdentifierChar (next))
        return true;

    if ((prev == '+' && next == '+') || (prev == '-' && next == '-'))
        return true;

    if ((prev == '&' && next == '&') || (prev == '|' && next == '|'))
        return true;

    if ((prev == '<' && next == '<') || (prev == '>' && next == '>'))
        return true;

    if (prev == '/' && (next == '/' || next == '*'))
        return true;

    if (prev == '*' && next == '/')
        return true;

    return false;
}

void appendMinifiedToken (std::string& output, const std::string& token)
{
    if (! output.empty())
    {
        const auto previous = output.back() == '\n'
                                ? std::string()
                                : std::string (1, output.back());

        if (needsSpaceBetween (previous, token))
            output.push_back (' ');
    }

    output += token;
}

juce::String fromUtf8 (const std::string& text)
{
    return juce::String::fromUTF8 (text.data(), static_cast<int> (text.size()));
}

std::string toUtf8String (const juce::String& text)
{
    const auto* raw = text.toRawUTF8();
    return { raw, std::strlen (raw) };
}

juce::int64 utf8Size (const juce::String& text)
{
    return static_cast<juce::int64> (std::strlen (text.toRawUTF8()));
}

WaviateShaderSerializer::SerializeResult serializeError (const juce::String& message)
{
    WaviateShaderSerializer::SerializeResult result;
    result.errorMessage = message;
    return result;
}

WaviateShaderSerializer::DeserializeResult deserializeError (const juce::String& message)
{
    WaviateShaderSerializer::DeserializeResult result;
    result.errorMessage = message;
    return result;
}

bool gzipCompress (const juce::String& text, juce::MemoryBlock& compressed)
{
    juce::MemoryOutputStream output;
    const auto* bytes = text.toRawUTF8();
    const auto byteCount = std::strlen (bytes);

    {
        juce::GZIPCompressorOutputStream gzipOutput (output, 9, juce::GZIPCompressorOutputStream::windowBitsGZIP);

        if (! gzipOutput.write (bytes, byteCount))
            return false;
    }

    compressed.replaceAll (output.getData(), output.getDataSize());
    return true;
}

bool gzipDecompress (const juce::MemoryBlock& compressed, juce::String& decompressed)
{
    juce::MemoryInputStream input (compressed.getData(), compressed.getSize(), false);
    juce::GZIPDecompressorInputStream gzipInput (&input, false, juce::GZIPDecompressorInputStream::gzipFormat);
    juce::MemoryOutputStream output;

    if (output.writeFromInputStream (gzipInput, -1) < 0)
        return false;

    decompressed = juce::String::fromUTF8 (static_cast<const char*> (output.getData()),
                                          static_cast<int> (output.getDataSize()));
    return true;
}

void setProperty (juce::DynamicObject& object, const char* name, const juce::var& value)
{
    object.setProperty (juce::Identifier (name), value);
}

juce::var getProperty (const juce::DynamicObject& object, const char* name)
{
    return object.getProperty (juce::Identifier (name));
}

bool readMetadata (const juce::DynamicObject& object, WaviateShaderSerializer::Metadata& metadata)
{
    metadata.schema = getProperty (object, "schema").toString();
    metadata.schemaVersion = static_cast<int> (getProperty (object, "schemaVersion"));
    metadata.language = getProperty (object, "language").toString();
    metadata.sourceEncoding = getProperty (object, "sourceEncoding").toString();
    metadata.minifier = getProperty (object, "minifier").toString();
    metadata.formatter = getProperty (object, "formatter").toString();
    metadata.compression = getProperty (object, "compression").toString();
    metadata.payloadEncoding = getProperty (object, "payloadEncoding").toString();
    metadata.originalSizeBytes = static_cast<juce::int64> (getProperty (object, "originalSizeBytes"));
    metadata.storedSourceSizeBytes = static_cast<juce::int64> (getProperty (object, "storedSourceSizeBytes"));
    metadata.payloadSizeBytes = static_cast<juce::int64> (getProperty (object, "payloadSizeBytes"));

    return metadata.schema == waviateShaderSchema && metadata.schemaVersion == waviateShaderSchemaVersion;
}

struct FormatToken
{
    enum class Kind
    {
        word,
        literal,
        comment,
        preprocessor,
        punctuator
    };

    Kind kind = Kind::punctuator;
    std::string text;
};

std::vector<FormatToken> lexForFormatting (const std::string& input)
{
    std::vector<FormatToken> tokens;
    auto i = size_t{};

    while (i < input.size())
    {
        if (isWhitespace (input[i]))
        {
            ++i;
            continue;
        }

        if (input[i] == '#')
        {
            const auto end = readPreprocessorDirective (input, i);
            tokens.push_back ({ FormatToken::Kind::preprocessor, trimRight (input.substr (i, end - i)) });
            i = end;
            continue;
        }

        if (input[i] == '/' && i + 1 < input.size())
        {
            if (input[i + 1] == '/')
            {
                const auto start = i;
                i += 2;

                while (i < input.size() && input[i] != '\n')
                    ++i;

                tokens.push_back ({ FormatToken::Kind::comment, trimRight (input.substr (start, i - start)) });
                continue;
            }

            if (input[i + 1] == '*')
            {
                const auto start = i;
                i += 2;

                while (i + 1 < input.size() && (input[i] != '*' || input[i + 1] != '/'))
                    ++i;

                i = std::min (input.size(), i + 2);
                tokens.push_back ({ FormatToken::Kind::comment, input.substr (start, i - start) });
                continue;
            }
        }

        if (const auto literalEnd = readStringLiteral (input, i); literalEnd != i)
        {
            tokens.push_back ({ FormatToken::Kind::literal, input.substr (i, literalEnd - i) });
            i = literalEnd;
            continue;
        }

        if (isIdentifierChar (input[i]))
        {
            const auto start = i++;

            while (i < input.size() && isIdentifierChar (input[i]))
                ++i;

            tokens.push_back ({ FormatToken::Kind::word, input.substr (start, i - start) });
            continue;
        }

        const auto punctuator = readPunctuator (input, i);
        tokens.push_back ({ FormatToken::Kind::punctuator, punctuator });
        i += punctuator.size();
    }

    return tokens;
}

bool isControlKeyword (const std::string& token)
{
    return token == "if" || token == "for" || token == "while" || token == "switch" || token == "catch";
}

bool isOperatorToken (const std::string& token)
{
    static constexpr const char* operators[] =
    {
        "=", "+", "-", "*", "/", "%", "==", "!=", "<", ">", "<=", ">=",
        "&&", "||", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<",
        ">>", "<<=", ">>=", "&", "|", "^", "?", ":"
    };

    return std::any_of (std::begin (operators), std::end (operators),
                        [&token] (const char* candidate) { return token == candidate; });
}

void trimFormattedLineRight (std::string& output)
{
    while (! output.empty() && (output.back() == ' ' || output.back() == '\t'))
        output.pop_back();
}

bool currentLineHasText (const std::string& output)
{
    const auto lastNewline = output.find_last_of ('\n');
    const auto start = lastNewline == std::string::npos ? 0 : lastNewline + 1;

    for (auto i = start; i < output.size(); ++i)
        if (! isWhitespace (output[i]))
            return true;

    return false;
}

void appendIndentIfNeeded (std::string& output, const int indent)
{
    if (! currentLineHasText (output))
        output += std::string (static_cast<size_t> (std::max (0, indent)) * 4, ' ');
}

void appendNewline (std::string& output)
{
    trimFormattedLineRight (output);

    if (output.empty() || output.back() != '\n')
        output.push_back ('\n');
}

void appendSpaceIfNeeded (std::string& output)
{
    if (! output.empty() && output.back() != ' ' && output.back() != '\n' && output.back() != '('
        && output.back() != '[' && output.back() != '.' && output.back() != '>')
        output.push_back (' ');
}

std::string formatTokens (const std::vector<FormatToken>& tokens)
{
    std::string output;
    std::string previousToken;
    auto indent = 0;
    auto parenDepth = 0;

    for (const auto& token : tokens)
    {
        const auto& text = token.text;

        if (token.kind == FormatToken::Kind::preprocessor)
        {
            appendNewline (output);
            output += text;
            appendNewline (output);
            previousToken = text;
            continue;
        }

        if (token.kind == FormatToken::Kind::comment)
        {
            appendIndentIfNeeded (output, indent);
            appendSpaceIfNeeded (output);
            output += text;

            if (text.rfind ("//", 0) == 0 || text.find ('\n') != std::string::npos)
                appendNewline (output);
            else
                output += " ";

            previousToken = text;
            continue;
        }

        if (previousToken == "}" && text != "else" && text != "catch" && text != "while" && text != ";")
            appendNewline (output);

        if (text == "{")
        {
            trimFormattedLineRight (output);
            appendSpaceIfNeeded (output);
            output += "{";
            appendNewline (output);
            ++indent;
        }
        else if (text == "}")
        {
            appendNewline (output);
            indent = std::max (0, indent - 1);
            appendIndentIfNeeded (output, indent);
            output += "}";
        }
        else if (text == ";")
        {
            trimFormattedLineRight (output);
            output += ";";

            if (parenDepth == 0)
                appendNewline (output);
            else
                output += " ";
        }
        else if (text == ",")
        {
            trimFormattedLineRight (output);
            output += ", ";
        }
        else if (text == "(")
        {
            appendIndentIfNeeded (output, indent);

            if (isControlKeyword (previousToken))
                appendSpaceIfNeeded (output);

            output += "(";
            ++parenDepth;
        }
        else if (text == ")")
        {
            trimFormattedLineRight (output);
            output += ")";
            parenDepth = std::max (0, parenDepth - 1);
        }
        else if (text == "[" || text == "]" || text == "." || text == "->" || text == "::")
        {
            trimFormattedLineRight (output);
            output += text;
        }
        else if (isOperatorToken (text))
        {
            trimFormattedLineRight (output);
            appendSpaceIfNeeded (output);
            output += text;
            output += " ";
        }
        else
        {
            appendIndentIfNeeded (output, indent);

            if (! previousToken.empty() && previousToken != "(" && previousToken != "[" && previousToken != "."
                && previousToken != "->" && previousToken != "::" && previousToken != "~")
                appendSpaceIfNeeded (output);

            output += text;
        }

        previousToken = text;
    }

    trimFormattedLineRight (output);

    if (! output.empty() && output.back() != '\n')
        output.push_back ('\n');

    return output;
}
} // namespace

WaviateShaderSerializer::SerializeResult WaviateShaderSerializer::serialize (const juce::String& source)
{
    return serialize (source, Options {});
}

WaviateShaderSerializer::SerializeResult WaviateShaderSerializer::serialize (const juce::String& source,
                                                                             const Options& options)
{
    const auto storedSource = options.minifySource ? minifyWlsl (source) : source;

    juce::String payload;
    juce::int64 payloadSize = 0;
    auto compression = juce::String ("none");
    auto payloadEncoding = juce::String ("utf8");

    if (options.compressPayload)
    {
        juce::MemoryBlock compressed;

        if (! gzipCompress (storedSource, compressed))
            return serializeError ("Could not gzip-compress Waviate shader source.");

        payload = juce::Base64::toBase64 (compressed.getData(), compressed.getSize());
        payloadSize = static_cast<juce::int64> (compressed.getSize());
        compression = "gzip";
        payloadEncoding = "base64";
    }
    else
    {
        payload = storedSource;
        payloadSize = utf8Size (storedSource);
    }

    juce::DynamicObject* root = new juce::DynamicObject();
    setProperty (*root, "schema", waviateShaderSchema);
    setProperty (*root, "schemaVersion", waviateShaderSchemaVersion);
    setProperty (*root, "language", "wlsl");
    setProperty (*root, "sourceEncoding", "utf-8");
    setProperty (*root, "minifier", options.minifySource ? "waviate-cpp-lex-v1" : "none");
    setProperty (*root, "formatter", "waviate-cpp-simple-v1");
    setProperty (*root, "compression", compression);
    setProperty (*root, "payloadEncoding", payloadEncoding);
    setProperty (*root, "originalSizeBytes", utf8Size (source));
    setProperty (*root, "storedSourceSizeBytes", utf8Size (storedSource));
    setProperty (*root, "payloadSizeBytes", payloadSize);
    setProperty (*root, "payload", payload);

    SerializeResult result;
    result.succeeded = true;
    result.serialized = juce::JSON::toString (juce::var (root), ! options.prettyPrintJson);
    return result;
}

WaviateShaderSerializer::DeserializeResult WaviateShaderSerializer::deserialize (const juce::String& serialized)
{
    return deserialize (serialized, Options {});
}

WaviateShaderSerializer::DeserializeResult WaviateShaderSerializer::deserialize (const juce::String& serialized,
                                                                                 const Options& options)
{
    juce::var parsed;

    if (const auto parseResult = juce::JSON::parse (serialized, parsed); parseResult.failed())
        return deserializeError ("Invalid Waviate shader JSON: " + parseResult.getErrorMessage());

    const auto* object = parsed.getDynamicObject();

    if (object == nullptr)
        return deserializeError ("Waviate shader payload must be a JSON object.");

    Metadata metadata;

    if (! readMetadata (*object, metadata))
        return deserializeError ("Unsupported Waviate shader schema or schema version.");

    const auto payload = getProperty (*object, "payload").toString();
    juce::String source;

    if (metadata.compression == "gzip" && metadata.payloadEncoding == "base64")
    {
        juce::MemoryOutputStream decoded;

        if (! juce::Base64::convertFromBase64 (decoded, payload))
            return deserializeError ("Waviate shader payload is not valid base64.");

        const juce::MemoryBlock compressed (decoded.getData(), decoded.getDataSize());

        if (! gzipDecompress (compressed, source))
            return deserializeError ("Could not gzip-decompress Waviate shader payload.");
    }
    else if (metadata.compression == "none" && metadata.payloadEncoding == "utf8")
    {
        source = payload;
    }
    else
    {
        return deserializeError ("Unsupported Waviate shader compression or payload encoding.");
    }

    DeserializeResult result;
    result.succeeded = true;
    result.metadata = metadata;
    result.source = options.formatOnDeserialize ? formatWlsl (source) : source;
    return result;
}

bool WaviateShaderSerializer::isSerializedShader (const juce::String& text)
{
    juce::var parsed;

    if (juce::JSON::parse (text, parsed).failed())
        return false;

    const auto* object = parsed.getDynamicObject();
    return object != nullptr
        && getProperty (*object, "schema").toString() == waviateShaderSchema
        && static_cast<int> (getProperty (*object, "schemaVersion")) == waviateShaderSchemaVersion;
}

juce::String WaviateShaderSerializer::minifyWlsl (const juce::String& source)
{
    const auto input = toUtf8String (source);
    std::string output;
    output.reserve (input.size());

    auto i = size_t {};
    auto atLineStart = true;

    while (i < input.size())
    {
        const auto c = input[i];

        if (isWhitespace (c))
        {
            if (c == '\n' || c == '\r')
                atLineStart = true;

            ++i;
            continue;
        }

        if (atLineStart && c == '#')
        {
            if (! output.empty() && output.back() != '\n')
                output.push_back ('\n');

            const auto end = readPreprocessorDirective (input, i);
            output += trimRight (input.substr (i, end - i));
            output.push_back ('\n');
            i = end;
            atLineStart = true;
            continue;
        }

        atLineStart = false;

        if (c == '/' && i + 1 < input.size())
        {
            if (input[i + 1] == '/')
            {
                const auto start = i;
                i += 2;

                while (i < input.size() && input[i] != '\n')
                    ++i;

                appendMinifiedToken (output, trimRight (input.substr (start, i - start)));
                output.push_back ('\n');
                atLineStart = true;
                continue;
            }

            if (input[i + 1] == '*')
            {
                const auto start = i;
                i += 2;

                while (i + 1 < input.size() && (input[i] != '*' || input[i + 1] != '/'))
                {
                    if (input[i] == '\n')
                        atLineStart = true;

                    ++i;
                }

                i = std::min (input.size(), i + 2);
                appendMinifiedToken (output, input.substr (start, i - start));
                continue;
            }
        }

        if (const auto literalEnd = readStringLiteral (input, i); literalEnd != i)
        {
            appendMinifiedToken (output, input.substr (i, literalEnd - i));
            i = literalEnd;
            continue;
        }

        if (isIdentifierChar (c))
        {
            const auto start = i++;

            while (i < input.size() && isIdentifierChar (input[i]))
                ++i;

            appendMinifiedToken (output, input.substr (start, i - start));
            continue;
        }

        if (isDigit (c))
        {
            const auto start = i++;

            while (i < input.size() && (isIdentifierChar (input[i]) || input[i] == '.'))
                ++i;

            appendMinifiedToken (output, input.substr (start, i - start));
            continue;
        }

        const auto punctuator = readPunctuator (input, i);
        appendMinifiedToken (output, punctuator);
        i += punctuator.size();
    }

    return fromUtf8 (trimRight (output));
}

juce::String WaviateShaderSerializer::formatWlsl (const juce::String& source)
{
    const auto minified = toUtf8String (minifyWlsl (source));
    return fromUtf8 (formatTokens (lexForFormatting (minified)));
}
