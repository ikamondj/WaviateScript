#pragma once

#include <JuceHeader.h>

class WaviateShaderSerializer final
{
public:
    struct Options
    {
        bool minifySource = true;
        bool compressPayload = true;
        bool formatOnDeserialize = true;
        bool prettyPrintJson = true;
    };

    struct Metadata
    {
        juce::String schema = "com.waviate.shader";
        int schemaVersion = 1;
        juce::String language = "wlsl";
        juce::String sourceEncoding = "utf-8";
        juce::String minifier = "waviate-cpp-lex-v1";
        juce::String formatter = "waviate-cpp-simple-v1";
        juce::String compression = "gzip";
        juce::String payloadEncoding = "base64";
        juce::int64 originalSizeBytes = 0;
        juce::int64 storedSourceSizeBytes = 0;
        juce::int64 payloadSizeBytes = 0;
    };

    struct SerializeResult
    {
        bool succeeded = false;
        juce::String serialized;
        juce::String errorMessage;

        explicit operator bool() const noexcept { return succeeded; }
    };

    struct DeserializeResult
    {
        bool succeeded = false;
        juce::String source;
        Metadata metadata;
        juce::String errorMessage;

        explicit operator bool() const noexcept { return succeeded; }
    };

    static SerializeResult serialize (const juce::String& source);
    static SerializeResult serialize (const juce::String& source, const Options& options);

    static DeserializeResult deserialize (const juce::String& serialized);
    static DeserializeResult deserialize (const juce::String& serialized, const Options& options);

    static bool isSerializedShader (const juce::String& text);

    static juce::String minifyWlsl (const juce::String& source);
    static juce::String formatWlsl (const juce::String& source);

private:
    WaviateShaderSerializer() = delete;
};
