#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <algorithm>

template <typename T, int Capacity, bool DropOldestOnOverflow = false>
class SpscEventQueue
{
    static_assert(Capacity > 0);

public:
    SpscEventQueue() : fifo(Capacity) {}

    int capacity() const noexcept { return Capacity; }

    int availableToRead() const noexcept { return fifo.getNumReady(); }
    int availableToWrite() const noexcept { return fifo.getFreeSpace(); }

    // Producer thread
    bool push(const T& item) noexcept
    {
        if constexpr (!DropOldestOnOverflow)
        {
            int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
            fifo.prepareToWrite(1, start1, size1, start2, size2);
            if (size1 == 0) return false;

            buffer[(size_t)start1] = item;
            fifo.finishedWrite(1);
            return true;
        }
        else
        {
            while (fifo.getFreeSpace() < 1)
                popOneNoCopy(); // drop oldest

            int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
            fifo.prepareToWrite(1, start1, size1, start2, size2);
            if (size1 == 0) return false;

            buffer[(size_t)start1] = item;
            fifo.finishedWrite(1);
            return true;
        }
    }

    // Producer thread: pushes up to numItems, returns number pushed
    int pushMany(const T* items, int numItems) noexcept
    {
        if (items == nullptr || numItems <= 0) return 0;

        if constexpr (!DropOldestOnOverflow)
        {
            numItems = std::min(numItems, fifo.getFreeSpace());
            if (numItems <= 0) return 0;

            int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
            fifo.prepareToWrite(numItems, start1, size1, start2, size2);

            for (int i = 0; i < size1; ++i)
                buffer[(size_t)(start1 + i)] = items[i];

            for (int i = 0; i < size2; ++i)
                buffer[(size_t)(start2 + i)] = items[size1 + i];

            fifo.finishedWrite(size1 + size2);
            return size1 + size2;
        }
        else
        {
            while (fifo.getFreeSpace() < numItems)
                popOneNoCopy(); // drop oldest until enough space (may be a lot)

            int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
            fifo.prepareToWrite(numItems, start1, size1, start2, size2);

            for (int i = 0; i < size1; ++i)
                buffer[(size_t)(start1 + i)] = items[i];

            for (int i = 0; i < size2; ++i)
                buffer[(size_t)(start2 + i)] = items[size1 + i];

            fifo.finishedWrite(size1 + size2);
            return size1 + size2;
        }
    }

    // Consumer thread (audio thread): pops up to maxItems, returns number popped
    int popMany(T* outItems, int maxItems) noexcept
    {
        if (outItems == nullptr || maxItems <= 0) return 0;

        maxItems = std::min(maxItems, fifo.getNumReady());
        if (maxItems <= 0) return 0;

        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        fifo.prepareToRead(maxItems, start1, size1, start2, size2);

        for (int i = 0; i < size1; ++i)
            outItems[i] = buffer[(size_t)(start1 + i)];

        for (int i = 0; i < size2; ++i)
            outItems[size1 + i] = buffer[(size_t)(start2 + i)];

        fifo.finishedRead(size1 + size2);
        return size1 + size2;
    }

    // Consumer thread (audio thread): call a functor for each popped item, returns count.
    // Signature: void fn(const T& item)
    template <typename Fn>
    int popEach(Fn&& fn, int maxItems) noexcept(noexcept(fn(std::declval<const T&>())))
    {
        maxItems = std::min(maxItems, fifo.getNumReady());
        if (maxItems <= 0) return 0;

        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        fifo.prepareToRead(maxItems, start1, size1, start2, size2);

        for (int i = 0; i < size1; ++i)
            fn(buffer[(size_t)(start1 + i)]);

        for (int i = 0; i < size2; ++i)
            fn(buffer[(size_t)(start2 + i)]);

        fifo.finishedRead(size1 + size2);
        return size1 + size2;
    }

    // Consumer thread: pop exactly one item (non-blocking). Returns false if empty.
    bool popOne(T& outItem) noexcept
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        fifo.prepareToRead(1, start1, size1, start2, size2);
        if (size1 == 0) return false;

        outItem = buffer[(size_t)start1];
        fifo.finishedRead(1);
        return true;
    }

    // Consumer thread: drop everything quickly.
    void clear() noexcept
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        const int n = fifo.getNumReady();
        if (n <= 0) return;
        fifo.prepareToRead(n, start1, size1, start2, size2);
        fifo.finishedRead(size1 + size2);
    }

private:
    void popOneNoCopy() noexcept
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        fifo.prepareToRead(1, start1, size1, start2, size2);
        if (size1 != 0)
            fifo.finishedRead(1);
    }

    juce::AbstractFifo fifo;
    std::array<T, (size_t)Capacity> buffer{};
};