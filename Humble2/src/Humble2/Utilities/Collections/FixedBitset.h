#pragma once

#include "Utilities\Allocators\Arena.h"

#include <cstdint>
#include <cstddef>
#include <cassert>
#include <memory>
#include <algorithm>

namespace HBL2
{
    class FixedBitset
    {
    public:
        using Word = std::uint64_t;
        static constexpr std::size_t BitsPerWord = 64;

        FixedBitset() = default;

        explicit FixedBitset(Arena* arena, std::size_t bitCount)
            : m_BitCount(bitCount), m_WordCount(wordsForBits(bitCount)), m_Arena(arena)
        {
            m_Words = (Word*)arena->Alloc(sizeof(Word) * m_WordCount);
            clearAll();
        }

        FixedBitset(const FixedBitset& other)
            : m_BitCount(other.m_BitCount), m_WordCount(other.m_WordCount), m_Arena(other.m_Arena)
        {
            m_Words = (Word*)m_Arena->Alloc(sizeof(Word) * m_WordCount);
            std::copy_n(other.m_Words, m_WordCount, m_Words);
        }

        FixedBitset& operator=(const FixedBitset& other)
        {
            if (this == &other)
            {
                return *this;
            }

            assert(m_BitCount == other.m_BitCount && "Bitsets must have same fixed size");
            std::copy_n(other.m_Words, m_WordCount, m_Words);
            return *this;
        }

        FixedBitset(FixedBitset&&) noexcept = default;
        FixedBitset& operator=(FixedBitset&&) noexcept = default;

        [[nodiscard]] inline std::size_t size() const noexcept
        {
            return m_BitCount;
        }

        [[nodiscard]] inline std::size_t wordCount() const noexcept
        {
            return m_WordCount;
        }

        [[nodiscard]] inline const Word* data() const noexcept
        {
            return m_Words;
        }

        [[nodiscard]] inline Word* data() noexcept
        {
            return m_Words;
        }

        inline void clearAll() noexcept
        {
            std::fill_n(m_Words, m_WordCount, Word{ 0 });
        }

        inline void setAll() noexcept
        {
            std::fill_n(m_Words, m_WordCount, ~Word{ 0 });
            clearUnusedTailBits();
        }

        inline void set(std::size_t pos) noexcept
        {
            assert(pos < m_BitCount);
            m_Words[wordIndex(pos)] |= bitMask(pos);
        }

        inline void reset(std::size_t pos) noexcept
        {
            assert(pos < m_BitCount);
            m_Words[wordIndex(pos)] &= ~bitMask(pos);
        }

        inline void set(std::size_t pos, bool value) noexcept
        {
            assert(pos < m_BitCount);
            Word& word = m_Words[wordIndex(pos)];
            const Word mask = bitMask(pos);

            // branchless
            word = (word & ~mask) | (Word(0) - Word(value)) & mask;
        }

        inline void flip(std::size_t pos) noexcept
        {
            assert(pos < m_BitCount);
            m_Words[wordIndex(pos)] ^= bitMask(pos);
        }

        [[nodiscard]] inline bool test(std::size_t pos) const noexcept
        {
            assert(pos < m_BitCount);
            return (m_Words[wordIndex(pos)] & bitMask(pos)) != 0;
        }

        [[nodiscard]] inline bool any() const noexcept
        {
            for (std::size_t i = 0; i < m_WordCount; ++i)
            {
                if (m_Words[i] != 0)
                {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] inline bool none() const noexcept
        {
            return !any();
        }

        [[nodiscard]] inline bool all() const noexcept
        {
            if (m_BitCount == 0)
            {
                return true;
            }

            const std::size_t fullWords = m_BitCount / BitsPerWord;
            for (std::size_t i = 0; i < fullWords; ++i)
            {
                if (m_Words[i] != ~Word{ 0 })
                {
                    return false;
                }
            }

            const std::size_t remBits = m_BitCount & (BitsPerWord - 1);
            if (remBits == 0)
            {
                return true;
            }

            const Word mask = (Word{ 1 } << remBits) - 1;
            return (m_Words[fullWords] & mask) == mask;
        }

        [[nodiscard]] inline bool intersects(const FixedBitset& other) const noexcept
        {
            assert(m_BitCount == other.m_BitCount);
            for (std::size_t i = 0; i < m_WordCount; ++i)
            {
                if ((m_Words[i] & other.m_Words[i]) != 0)
                {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] inline bool contains(const FixedBitset& other) const noexcept
        {
            assert(m_BitCount == other.m_BitCount);
            for (std::size_t i = 0; i < m_WordCount; ++i)
            {
                if ((m_Words[i] & other.m_Words[i]) != other.m_Words[i])
                {
                    return false;
                }
            }
            return true;
        }

        inline FixedBitset& operator&=(const FixedBitset& other) noexcept
        {
            assert(m_BitCount == other.m_BitCount);
            for (std::size_t i = 0; i < m_WordCount; ++i)
            {
                m_Words[i] &= other.m_Words[i];
            }

            return *this;
        }

        inline FixedBitset& operator|=(const FixedBitset& other) noexcept
        {
            assert(m_BitCount == other.m_BitCount);
            for (std::size_t i = 0; i < m_WordCount; ++i)
            {
                m_Words[i] |= other.m_Words[i];
            }

            return *this;
        }

        inline FixedBitset& operator^=(const FixedBitset& other) noexcept
        {
            assert(m_BitCount == other.m_BitCount);
            for (std::size_t i = 0; i < m_WordCount; ++i)
            {
                m_Words[i] ^= other.m_Words[i];
            }

            clearUnusedTailBits();
            return *this;
        }

        [[nodiscard]] inline FixedBitset operator~() const
        {
            FixedBitset result(*this);
            for (std::size_t i = 0; i < m_WordCount; ++i)
            {
                result.m_Words[i] = ~result.m_Words[i];
            }

            result.clearUnusedTailBits();
            return result;
        }

        [[nodiscard]] inline bool operator==(const FixedBitset& other) const noexcept
        {
            if (m_BitCount != other.m_BitCount)
            {
                return false;
            }

            for (std::size_t i = 0; i < m_WordCount; ++i)
            {
                if (m_Words[i] != other.m_Words[i])
                {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] inline bool operator!=(const FixedBitset& other) const noexcept
        {
            return !(*this == other);
        }

        [[nodiscard]] friend inline FixedBitset operator&(FixedBitset lhs, const FixedBitset& rhs) noexcept
        {
            lhs &= rhs;
            return lhs;
        }

        [[nodiscard]] friend inline FixedBitset operator|(FixedBitset lhs, const FixedBitset& rhs) noexcept
        {
            lhs |= rhs;
            return lhs;
        }

        [[nodiscard]] friend inline FixedBitset operator^(FixedBitset lhs, const FixedBitset& rhs) noexcept
        {
            lhs ^= rhs;
            return lhs;
        }

    private:
        std::size_t m_BitCount;
        std::size_t m_WordCount;
        Word* m_Words = nullptr;
        Arena* m_Arena = nullptr;

        [[nodiscard]] static constexpr std::size_t wordsForBits(std::size_t bits) noexcept
        {
            return (bits + BitsPerWord - 1) / BitsPerWord;
        }

        [[nodiscard]] static constexpr std::size_t wordIndex(std::size_t bit) noexcept
        {
            return bit >> 6;
        }

        [[nodiscard]] static constexpr Word bitMask(std::size_t bit) noexcept
        {
            return Word{ 1 } << (bit & 63);
        }

        inline void clearUnusedTailBits() noexcept
        {
            const std::size_t remBits = m_BitCount & (BitsPerWord - 1);
            if (remBits == 0 || m_WordCount == 0)
            {
                return;
            }

            const Word mask = (Word{ 1 } << remBits) - 1;
            m_Words[m_WordCount - 1] &= mask;
        }
    };
}