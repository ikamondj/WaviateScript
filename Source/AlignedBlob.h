/*
  ==============================================================================

    AlignedBlob.h
    Created: 12 Jan 2026 9:12:43pm
    Author:  ikamo

  ==============================================================================
*/

#pragma once

#include <cstddef>

class AlignedBlob {
public:
    AlignedBlob() noexcept;

    AlignedBlob(std::size_t bytes, std::size_t alignment);

    ~AlignedBlob();

    AlignedBlob(const AlignedBlob&) = delete;
    AlignedBlob& operator=(const AlignedBlob&) = delete;

    AlignedBlob(AlignedBlob&& other) noexcept;
    AlignedBlob& operator=(AlignedBlob&& other) noexcept;

    void* data() const noexcept;
    std::size_t size() const noexcept;
    std::size_t alignment() const noexcept;

    void reset() noexcept;

private:
    void moveFrom(AlignedBlob& other) noexcept;

    void* ptr_ = nullptr;
    std::size_t size_ = 0;
    std::size_t align_ = 0;
};
