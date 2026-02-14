/*
  ==============================================================================

    AlignedBlob.cpp
    Created: 12 Jan 2026 9:12:43pm
    Author:  ikamo

  ==============================================================================
*/

#include "AlignedBlob.h"

#include <new>
#include <cstring>
#include <algorithm>

static bool isPowerOfTwo(std::size_t x)
{
    return x && ((x & (x - 1)) == 0);
}

AlignedBlob::AlignedBlob() noexcept = default;

AlignedBlob::AlignedBlob(std::size_t bytes, std::size_t alignment)
    : size_(bytes), align_(alignment)
{
    if (size_ == 0)
        return;

    if (!isPowerOfTwo(align_))
        align_ = alignof(std::max_align_t);

    align_ = std::max(align_, alignof(std::max_align_t));

    ptr_ = ::operator new(size_, std::align_val_t(align_));
    std::memset(ptr_, 0, size_);
}

AlignedBlob::~AlignedBlob()
{
    reset();
}

AlignedBlob::AlignedBlob(AlignedBlob&& other) noexcept
{
    moveFrom(other);
}

AlignedBlob& AlignedBlob::operator=(AlignedBlob&& other) noexcept
{
    if (this != &other) {
        reset();
        moveFrom(other);
    }
    return *this;
}

void* AlignedBlob::data() const noexcept
{
    return ptr_;
}

std::size_t AlignedBlob::size() const noexcept
{
    return size_;
}

std::size_t AlignedBlob::alignment() const noexcept
{
    return align_;
}

void AlignedBlob::reset() noexcept
{
    if (ptr_) {
        ::operator delete(ptr_, std::align_val_t(align_));
        ptr_ = nullptr;
    }
    size_ = 0;
    align_ = 0;
}

void AlignedBlob::moveFrom(AlignedBlob& other) noexcept
{
    ptr_ = other.ptr_;
    size_ = other.size_;
    align_ = other.align_;

    other.ptr_ = nullptr;
    other.size_ = 0;
    other.align_ = 0;
}
