#include "MarketplaceClient.h"

namespace
{
    constexpr const char* marketplaceBaseURLSettingKey = "marketplaceBaseUrl";
    constexpr const char* marketplaceSessionTokenSettingKey = "marketplaceSessionToken";
    constexpr const char* marketplaceSessionExpiresAtSettingKey = "marketplaceSessionExpiresAt";
    constexpr const char* defaultMarketplaceBaseURL = "http://localhost:8080";
}

MarketplaceClient::MarketplaceClient(juce::PropertiesFile& settingsRef)
    : settings(settingsRef)
{
}

bool MarketplaceClient::hasValidSession() const
{
    return getSessionToken().isNotEmpty()
        && getSessionExpiry().toMilliseconds() > juce::Time::getCurrentTime().toMilliseconds();
}

juce::String MarketplaceClient::getLoginURL() const
{
    return getBaseURL() + "/api/auth/google/start?client=desktop";
}

juce::String MarketplaceClient::getSessionExpiryText() const
{
    const auto expiry = getSessionExpiry();
    if (expiry.toMilliseconds() <= 0)
        return {};

    return expiry.toISO8601(true);
}

bool MarketplaceClient::launchLogin() const
{
    return juce::URL(getLoginURL()).launchInDefaultBrowser();
}

bool MarketplaceClient::storeSessionPaste(const juce::String& pastedSession, juce::String& errorMessage)
{
    const auto trimmed = pastedSession.trim();
    const auto separator = trimmed.indexOfChar('|');
    if (separator <= 0 || separator >= trimmed.length() - 1)
    {
        errorMessage = "Paste the full session value from the browser.";
        return false;
    }

    const auto token = trimmed.substring(0, separator).trim();
    const auto expiresAt = juce::Time::fromISO8601(trimmed.substring(separator + 1).trim());
    if (token.isEmpty())
    {
        errorMessage = "The session token was empty.";
        return false;
    }
    if (expiresAt.toMilliseconds() <= juce::Time::getCurrentTime().toMilliseconds())
    {
        errorMessage = "That marketplace session is already expired.";
        return false;
    }

    settings.setValue(marketplaceSessionTokenSettingKey, token);
    settings.setValue(marketplaceSessionExpiresAtSettingKey, expiresAt.toISO8601(true));
    settings.saveIfNeeded();
    return true;
}

void MarketplaceClient::clearSession()
{
    settings.removeValue(marketplaceSessionTokenSettingKey);
    settings.removeValue(marketplaceSessionExpiresAtSettingKey);
    settings.saveIfNeeded();
}

MarketplaceClient::UploadResult MarketplaceClient::uploadScript(const juce::String& name,
                                                                const juce::String& description,
                                                                const juce::String& content,
                                                                const juce::StringArray& tags,
                                                                bool requiresPremium) const
{
    if (! hasValidSession())
        return { false, "Log in to upload marketplace scripts." };

    const auto body = juce::String("{")
        + "\"name\":" + jsonString(name) + ","
        + "\"description\":" + jsonString(description) + ","
        + "\"requiresPremium\":" + (requiresPremium ? "true" : "false") + ","
        + "\"content\":" + jsonString(content) + ","
        + "\"tags\":" + jsonStringArray(tags)
        + "}";

    int statusCode = 0;
    auto request = juce::URL(getBaseURL() + "/api/uploads").withPOSTData(body);
    auto stream = request.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withHttpRequestCmd("POST")
            .withConnectionTimeoutMs(12000)
            .withStatusCode(&statusCode)
            .withExtraHeaders("Content-Type: application/json\r\n"
                              "Accept: application/json\r\n"
                              "Authorization: Bearer " + getSessionToken() + "\r\n"));

    if (stream == nullptr)
        return { false, "Could not reach the marketplace backend." };

    const auto responseBody = stream->readEntireStreamAsString();
    if (statusCode >= 200 && statusCode < 300)
        return { true, "Upload accepted by the marketplace." };

    auto errorMessage = responseBody.trim();
    if (auto parsed = juce::JSON::parse(responseBody); parsed.isObject())
    {
        if (auto* object = parsed.getDynamicObject())
            errorMessage = object->getProperty("error").toString().trim();
    }
    if (errorMessage.isEmpty())
        errorMessage = "Marketplace upload failed with HTTP " + juce::String(statusCode) + ".";

    return { false, errorMessage };
}

juce::String MarketplaceClient::getBaseURL() const
{
    return settings.getValue(marketplaceBaseURLSettingKey, defaultMarketplaceBaseURL).trim().trimCharactersAtEnd("/");
}

juce::String MarketplaceClient::getSessionToken() const
{
    return settings.getValue(marketplaceSessionTokenSettingKey).trim();
}

juce::Time MarketplaceClient::getSessionExpiry() const
{
    return juce::Time::fromISO8601(settings.getValue(marketplaceSessionExpiresAtSettingKey));
}

juce::String MarketplaceClient::jsonString(const juce::String& value)
{
    return "\"" + juce::JSON::escapeString(value) + "\"";
}

juce::String MarketplaceClient::jsonStringArray(const juce::StringArray& values)
{
    juce::StringArray quoted;
    for (const auto& value : values)
        quoted.add(jsonString(value));

    return "[" + quoted.joinIntoString(",") + "]";
}
