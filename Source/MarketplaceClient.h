#pragma once

#include <JuceHeader.h>

class MarketplaceClient
{
public:
    struct UploadResult
    {
        bool succeeded = false;
        juce::String message;
    };

    explicit MarketplaceClient(juce::PropertiesFile& settings);

    [[nodiscard]] bool hasValidSession() const;
    [[nodiscard]] juce::String getLoginURL() const;
    [[nodiscard]] juce::String getSessionExpiryText() const;

    bool launchLogin() const;
    bool storeSessionPaste(const juce::String& pastedSession, juce::String& errorMessage);
    void clearSession();

    UploadResult uploadScript(const juce::String& name,
                              const juce::String& description,
                              const juce::String& content,
                              const juce::StringArray& tags,
                              bool requiresPremium) const;

private:
    [[nodiscard]] juce::String getBaseURL() const;
    [[nodiscard]] juce::String getSessionToken() const;
    [[nodiscard]] juce::Time getSessionExpiry() const;
    static juce::String jsonString(const juce::String& value);
    static juce::String jsonStringArray(const juce::StringArray& values);

    juce::PropertiesFile& settings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarketplaceClient)
};
