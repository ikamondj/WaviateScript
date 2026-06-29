#include "WaviateAudio.h"

#include <algorithm>
#include <utility>

namespace waviate::audio
{
namespace
{
thread_local WaviateAudioCache* currentThreadAudioCache = nullptr;

std::string normalizeLocation(std::string_view location)
{
    const auto first = location.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos)
        return {};

    const auto last = location.find_last_not_of(" \t\r\n");
    return std::string(location.substr(first, last - first + 1));
}
} // namespace

WaviateAudioCache::LoadedAudio::LoadedAudio(std::vector<float> samplesIn,
                                            int32_t channelCountIn,
                                            float sampleRateIn)
    : samples(std::move(samplesIn))
{
    const auto safeChannelCount = std::max(1, channelCountIn);
    clip.samples = samples.data();
    clip.frameCount = static_cast<uint64_t>(samples.size() / static_cast<size_t>(safeChannelCount));
    clip.channelCount = safeChannelCount;
    clip.sampleRate = sampleRateIn > 0.0f ? sampleRateIn : 0.0f;
}

const WaviateAudioRuntimeClip* WaviateAudioCache::requestAudio(std::string_view location)
{
    const auto normalizedLocation = normalizeLocation(location);
    if (normalizedLocation.empty())
        return nullptr;

    bool shouldNotify = false;
    {
        std::lock_guard lock(mutex_);

        if (auto found = entries_.find(normalizedLocation); found != entries_.end())
        {
            const auto& entry = found->second;
            if (entry.state == WaviateAudioLoadState::Ready && entry.loaded != nullptr)
                return &entry.loaded->clip;

            return nullptr;
        }

        entries_.emplace(normalizedLocation, Entry {});
        pendingRequests_.push_back(normalizedLocation);
        shouldNotify = true;
    }

    if (shouldNotify && onRequestAdded)
        onRequestAdded();

    return nullptr;
}

bool WaviateAudioCache::popPendingRequest(WaviateAudioLoadRequest& request)
{
    std::lock_guard lock(mutex_);

    if (pendingRequests_.empty())
        return false;

    request.location = std::move(pendingRequests_.front());
    pendingRequests_.pop_front();

    if (auto found = entries_.find(request.location); found != entries_.end())
        found->second.state = WaviateAudioLoadState::Loading;

    return true;
}

std::vector<WaviateAudioLoadRequest> WaviateAudioCache::drainPendingRequests()
{
    std::vector<WaviateAudioLoadRequest> requests;
    std::lock_guard lock(mutex_);
    requests.reserve(pendingRequests_.size());

    while (! pendingRequests_.empty())
    {
        auto location = std::move(pendingRequests_.front());
        pendingRequests_.pop_front();

        if (auto found = entries_.find(location); found != entries_.end())
            found->second.state = WaviateAudioLoadState::Loading;

        requests.push_back({ std::move(location) });
    }

    return requests;
}

void WaviateAudioCache::markLoading(std::string_view location)
{
    const auto normalizedLocation = normalizeLocation(location);
    if (normalizedLocation.empty())
        return;

    std::lock_guard lock(mutex_);
    auto [it, inserted] = entries_.try_emplace(normalizedLocation);
    static_cast<void>(inserted);
    it->second.state = WaviateAudioLoadState::Loading;
    it->second.errorMessage.clear();
}

bool WaviateAudioCache::storeLoadedAudio(std::string location,
                                         std::vector<float> interleavedSamples,
                                         int32_t channelCount,
                                         float sampleRate)
{
    location = normalizeLocation(location);
    if (location.empty() || channelCount <= 0 || interleavedSamples.empty())
        return false;

    const auto frameCount = interleavedSamples.size() / static_cast<size_t>(channelCount);
    if (frameCount == 0)
        return false;

    interleavedSamples.resize(frameCount * static_cast<size_t>(channelCount));
    auto loaded = std::make_shared<LoadedAudio>(std::move(interleavedSamples), channelCount, sampleRate);

    std::lock_guard lock(mutex_);
    pendingRequests_.erase(std::remove(pendingRequests_.begin(), pendingRequests_.end(), location),
                           pendingRequests_.end());

    auto& entry = entries_[location];
    entry.state = WaviateAudioLoadState::Ready;
    entry.loaded = std::move(loaded);
    entry.errorMessage.clear();
    return true;
}

void WaviateAudioCache::markFailed(std::string_view location, std::string errorMessage)
{
    const auto normalizedLocation = normalizeLocation(location);
    if (normalizedLocation.empty())
        return;

    std::lock_guard lock(mutex_);
    pendingRequests_.erase(std::remove(pendingRequests_.begin(), pendingRequests_.end(), normalizedLocation),
                           pendingRequests_.end());

    auto& entry = entries_[normalizedLocation];
    entry.state = WaviateAudioLoadState::Failed;
    entry.loaded.reset();
    entry.errorMessage = std::move(errorMessage);
}

void WaviateAudioCache::clear()
{
    std::lock_guard lock(mutex_);
    entries_.clear();
    pendingRequests_.clear();
}

std::vector<WaviateAudioCacheEntryInfo> WaviateAudioCache::snapshot() const
{
    std::vector<WaviateAudioCacheEntryInfo> result;
    std::lock_guard lock(mutex_);
    result.reserve(entries_.size());

    for (const auto& [location, entry] : entries_)
        result.push_back(makeInfo(location, entry));

    std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.location < rhs.location;
    });

    return result;
}

size_t WaviateAudioCache::loadedBytes() const
{
    size_t bytes = 0;
    std::lock_guard lock(mutex_);

    for (const auto& [location, entry] : entries_)
    {
        static_cast<void>(location);
        if (entry.loaded != nullptr)
            bytes += entry.loaded->samples.size() * sizeof(float);
    }

    return bytes;
}

size_t WaviateAudioCache::pendingRequestCount() const
{
    std::lock_guard lock(mutex_);
    return pendingRequests_.size();
}

WaviateAudioCacheEntryInfo WaviateAudioCache::makeInfo(const std::string& location, const Entry& entry)
{
    WaviateAudioCacheEntryInfo info;
    info.location = location;
    info.state = entry.state;
    info.errorMessage = entry.errorMessage;
    info.customName = entry.customName;
    info.isManual = entry.isManual;

    if (entry.loaded != nullptr)
    {
        info.frameCount = entry.loaded->clip.frameCount;
        info.channelCount = entry.loaded->clip.channelCount;
        info.sampleRate = entry.loaded->clip.sampleRate;
        info.byteSize = entry.loaded->samples.size() * sizeof(float);
    }

    return info;
}

void WaviateAudioCache::registerManualClip(std::string_view location, std::string_view customName)
{
    const auto normalizedLocation = normalizeLocation(location);
    if (normalizedLocation.empty())
        return;

    bool shouldNotify = false;
    {
        std::lock_guard lock(mutex_);
        auto [it, inserted] = entries_.try_emplace(normalizedLocation);
        it->second.customName = std::string(customName);
        it->second.isManual = true;
        
        if (inserted || it->second.state == WaviateAudioLoadState::Failed)
        {
            it->second.state = WaviateAudioLoadState::Pending;
            pendingRequests_.push_back(normalizedLocation);
            shouldNotify = true;
        }
    }

    if (shouldNotify && onRequestAdded)
        onRequestAdded();
}

void WaviateAudioCache::removeManualClip(std::string_view location)
{
    const auto normalizedLocation = normalizeLocation(location);
    if (normalizedLocation.empty())
        return;

    std::lock_guard lock(mutex_);
    entries_.erase(normalizedLocation);
    pendingRequests_.erase(std::remove(pendingRequests_.begin(), pendingRequests_.end(), normalizedLocation),
                           pendingRequests_.end());
}

void WaviateAudioCache::setClipName(std::string_view location, std::string_view customName)
{
    const auto normalizedLocation = normalizeLocation(location);
    if (normalizedLocation.empty())
        return;

    std::lock_guard lock(mutex_);
    if (auto found = entries_.find(normalizedLocation); found != entries_.end())
    {
        found->second.customName = std::string(customName);
    }
}

void setCurrentThreadAudioCache(WaviateAudioCache* cache) noexcept
{
    currentThreadAudioCache = cache;
}

WaviateAudioCache* getCurrentThreadAudioCache() noexcept
{
    return currentThreadAudioCache;
}

ScopedAudioCacheBinding::ScopedAudioCacheBinding(WaviateAudioCache* cache) noexcept
    : previous_(getCurrentThreadAudioCache())
{
    setCurrentThreadAudioCache(cache);
}

ScopedAudioCacheBinding::~ScopedAudioCacheBinding()
{
    setCurrentThreadAudioCache(previous_);
}

#include <cstdio>

const WaviateAudioRuntimeClip* waviate_load_audio_from_location(const char* location) noexcept
{
    std::printf("[DEBUG] waviate_load_audio_from_location: location=%p\n", (void*)location);
    if (location != nullptr)
        std::printf("[DEBUG] waviate_load_audio_from_location value: %s\n", location);
    if (location == nullptr)
        return nullptr;

    try
    {
        auto* cache = getCurrentThreadAudioCache();
        std::printf("[DEBUG] waviate_load_audio_from_location: cache=%p\n", (void*)cache);
        auto* result = cache != nullptr ? cache->requestAudio(location) : nullptr;
        std::printf("[DEBUG] waviate_load_audio_from_location: result=%p\n", (void*)result);
        return result;
    }
    catch (...)
    {
        std::printf("[DEBUG] waviate_load_audio_from_location: caught exception\n");
        return nullptr;
    }
}
} // namespace waviate::audio
