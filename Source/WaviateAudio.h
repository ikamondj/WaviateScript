#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct WaviateAudioRuntimeClip
{
    const float* samples = nullptr;
    uint64_t frameCount = 0;
    int32_t channelCount = 0;
    float sampleRate = 0.0f;
};

namespace waviate::audio
{
enum class WaviateAudioLoadState : uint32_t
{
    Pending = 0,
    Loading,
    Ready,
    Failed
};

struct WaviateAudioLoadRequest
{
    std::string location;
};

struct WaviateAudioCacheEntryInfo
{
    std::string location;
    WaviateAudioLoadState state = WaviateAudioLoadState::Pending;
    uint64_t frameCount = 0;
    int32_t channelCount = 0;
    float sampleRate = 0.0f;
    size_t byteSize = 0;
    std::string errorMessage;
    std::string customName;
    bool isManual = false;
};

class WaviateAudioCache
{
public:
    [[nodiscard]] const WaviateAudioRuntimeClip* requestAudio(std::string_view location);
    [[nodiscard]] bool popPendingRequest(WaviateAudioLoadRequest& request);
    [[nodiscard]] std::vector<WaviateAudioLoadRequest> drainPendingRequests();

    void markLoading(std::string_view location);
    [[nodiscard]] bool storeLoadedAudio(std::string location,
                                        std::vector<float> interleavedSamples,
                                        int32_t channelCount,
                                        float sampleRate);
    void markFailed(std::string_view location, std::string errorMessage);
    void clear();

    [[nodiscard]] std::vector<WaviateAudioCacheEntryInfo> snapshot() const;
    [[nodiscard]] size_t loadedBytes() const;
    [[nodiscard]] size_t pendingRequestCount() const;

    void registerManualClip(std::string_view location, std::string_view customName);
    void removeManualClip(std::string_view location);
    void setClipName(std::string_view location, std::string_view customName);

    std::function<void()> onRequestAdded;

private:
    struct TransparentStringHash
    {
        using is_transparent = void;

        [[nodiscard]] size_t operator()(std::string_view value) const noexcept
        {
            return std::hash<std::string_view> {}(value);
        }

        [[nodiscard]] size_t operator()(const std::string& value) const noexcept
        {
            return (*this)(std::string_view(value));
        }

        [[nodiscard]] size_t operator()(const char* value) const noexcept
        {
            return (*this)(std::string_view(value != nullptr ? value : ""));
        }
    };

    struct TransparentStringEqual
    {
        using is_transparent = void;

        [[nodiscard]] bool operator()(std::string_view lhs, std::string_view rhs) const noexcept
        {
            return lhs == rhs;
        }
    };

    struct LoadedAudio
    {
        LoadedAudio(std::vector<float> samplesIn, int32_t channelCountIn, float sampleRateIn);

        std::vector<float> samples;
        WaviateAudioRuntimeClip clip;
    };

    struct Entry
    {
        WaviateAudioLoadState state = WaviateAudioLoadState::Pending;
        std::shared_ptr<LoadedAudio> loaded;
        std::string errorMessage;
        std::string customName;
        bool isManual = false;
    };

    using EntryMap = std::unordered_map<std::string, Entry, TransparentStringHash, TransparentStringEqual>;

    [[nodiscard]] static WaviateAudioCacheEntryInfo makeInfo(const std::string& location, const Entry& entry);

    mutable std::mutex mutex_;
    EntryMap entries_;
    std::deque<std::string> pendingRequests_;
};

void setCurrentThreadAudioCache(WaviateAudioCache* cache) noexcept;
[[nodiscard]] WaviateAudioCache* getCurrentThreadAudioCache() noexcept;

class ScopedAudioCacheBinding final
{
public:
    explicit ScopedAudioCacheBinding(WaviateAudioCache* cache) noexcept;
    ~ScopedAudioCacheBinding();

    ScopedAudioCacheBinding(const ScopedAudioCacheBinding&) = delete;
    ScopedAudioCacheBinding& operator=(const ScopedAudioCacheBinding&) = delete;

private:
    WaviateAudioCache* previous_ = nullptr;
};

extern "C" const WaviateAudioRuntimeClip* waviate_load_audio_from_location(const char* location) noexcept;
} // namespace waviate::audio
