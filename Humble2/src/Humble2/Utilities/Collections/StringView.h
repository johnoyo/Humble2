#pragma once

#include "String.h"

#include <cstddef>
#include <cstring>
#include <string>
#include <algorithm>

namespace HBL2
{
    /**
     * @brief A lightweight, non-owning view of a contiguous character sequence, similar to std::string_view.
     * 
     * It provides read-only access to string data without copying,
     * making it ideal for efficient string manipulation and function parameter passing.
     */
    class StringView
    {
    public:
        /**
         * @brief Default constructor creates an empty view.
         */
        constexpr StringView() noexcept : m_Data(""), m_Length(0) {}

        /**
         * @brief Constructor from a null-terminated C-style string.
         *
         * @param str Pointer to a null-terminated C-style string.
         */
        StringView(const char* str)
            : m_Data(str), m_Length(str ? std::strlen(str) : 0) {}

        /**
         * @brief Constructor from a char pointer and length.
         *
         * @param str Pointer to the character data.
         * @param len Length of the character sequence.
         */
        constexpr StringView(const char* str, uint64_t len)
            : m_Data(str), m_Length(len) {}

        /**
         * @brief Constructor from std::string.
         *
         * @param str The std::string to create a view from.
         */
        StringView(const std::string& str)
            : m_Data(str.data()), m_Length(str.length()) {}

        /**
         * @brief Constructor from your custom String class.
         *
         * @tparam TAllocator Allocator type used by the String.
         * @param str The custom String to create a view from.
         */
        template<typename TAllocator>
        StringView(const String<TAllocator>& str)
            : m_Data(str.Data()), m_Length(str.Length()) {}

        /**
         * @brief Returns a pointer to the underlying data.
         *
         * @return Pointer to the character data.
         */
        constexpr const char* Data() const noexcept { return m_Data; }

        /**
         * @brief Returns the number of characters in the view.
         *
         * @return Number of characters.
         */
        constexpr uint64_t Length() const noexcept { return m_Length; }

        /**
         * @brief Returns true if the view is empty.
         *
         * @return True if empty, false otherwise.
         */
        constexpr bool Empty() const noexcept { return m_Length == 0; }

        /**
         * @brief Converts to std::string.
         *
         * @return A std::string copy of the view.
         */
        std::string ToStdString() const { return std::string(m_Data, m_Length); }

        /**
         * @brief Converts the view to a full String using a provided allocator.
         *
         * @tparam TAllocator Allocator type to use.
         * @param allocator Pointer to allocator instance (default nullptr).
         * @return A new String constructed from the view.
         */
        template<typename TAllocator = StandardAllocator>
        String<TAllocator> ToString(TAllocator* allocator = nullptr) const
        {
            return String<TAllocator>(allocator, m_Data, m_Length);
        }

        /**
         * @brief Access character at the specified index.
         *
         * @param index Index of the character to access.
         * @return Reference to the character at the specified index, or null character if out of range.
         */
        constexpr const char& operator[](uint64_t index) const
        {
            return (index < m_Length) ? m_Data[index] : s_NullChar;
        }

        /**
         * @brief Front character.
         *
         * @return The first character, or null character if empty.
         */
        constexpr char Front() const { return m_Length > 0 ? m_Data[0] : s_NullChar; }

        /**
         * @brief Back character.
         *
         * @return The last character, or null character if empty.
         */
        constexpr char Back() const { return m_Length > 0 ? m_Data[m_Length - 1] : s_NullChar; }

        /**
         * @brief Remove prefix of N characters.
         *
         * @param n Number of characters to remove from the start.
         */
        void RemovePrefix(uint64_t n)
        {
            n = std::min(n, m_Length);
            m_Data += n;
            m_Length -= n;
        }

        /**
         * @brief Remove suffix of N characters.
         *
         * @param n Number of characters to remove from the end.
         */
        void RemoveSuffix(uint64_t n)
        {
            m_Length = (n >= m_Length) ? 0 : m_Length - n;
        }

        /**
         * @brief Create a substring view from this view.
         *
         * @param start Starting index of the substring.
         * @param count Number of characters in the substring (default is npos, meaning until the end).
         * @return A new StringView representing the substring.
         */
        StringView SubString(uint64_t start, uint64_t count = npos) const
        {
            if (start >= m_Length)
            {
                return {};
            }

            uint64_t len = std::min(count, m_Length - start);
            return StringView(m_Data + start, len);
        }

        /**
         * @brief Find substring in view.
         *
         * @param substr Null-terminated substring to find.
         * @param pos Position to start searching from (default 0).
         * @return Index of the first occurrence or npos if not found.
         */
        uint64_t Find(const char* substr, uint64_t pos = 0) const
        {
            if (pos >= m_Length || substr == nullptr)
            {
                return npos;
            }

            const char* found = std::strstr(m_Data + pos, substr);
            return (found && found < m_Data + m_Length) ? static_cast<uint64_t>(found - m_Data) : npos;
        }

        /**
         * @brief Find first occurrence of a character.
         *
         * @param ch Character to find.
         * @param pos Position to start searching from (default 0).
         * @return Index of the first occurrence or npos if not found.
         */
        uint64_t Find(char ch, uint64_t pos = 0) const
        {
            for (uint64_t i = pos; i < m_Length; ++i)
            {
                if (m_Data[i] == ch)
                {
                    return i;
                }
            }
            return npos;
        }

        /**
         * @brief Returns true if the view starts with the given string.
         *
         * @param prefix The prefix StringView to check.
         * @return True if the view starts with prefix, false otherwise.
         */
        bool StartsWith(const StringView& prefix) const
        {
            if (prefix.m_Length > m_Length)
            {
                return false;
            }

            return std::strncmp(m_Data, prefix.m_Data, prefix.m_Length) == 0;
        }

        /**
         * @brief Returns true if the view starts with the given character.
         *
         * @param ch The character to check.
         * @return True if the view starts with ch, false otherwise.
         */
        bool StartsWith(char ch) const
        {
            return m_Length > 0 && m_Data[0] == ch;
        }

        /**
         * @brief Returns true if the view ends with the given string.
         *
         * @param suffix The suffix StringView to check.
         * @return True if the view ends with suffix, false otherwise.
         */
        bool EndsWith(const StringView& suffix) const
        {
            if (suffix.m_Length > m_Length)
            {
                return false;
            }

            return std::strncmp(m_Data + m_Length - suffix.m_Length, suffix.m_Data, suffix.m_Length) == 0;
        }

        /**
         * @brief Returns true if the view ends with the given character.
         *
         * @param ch The character to check.
         * @return True if the view ends with ch, false otherwise.
         */
        bool EndsWith(char ch) const
        {
            return m_Length > 0 && m_Data[m_Length - 1] == ch;
        }

        /**
         * @brief Returns true if the view contains the given substring.
         *
         * @param target The substring to check for.
         * @return True if the substring is found, false otherwise.
         */
        bool Contains(const StringView& target) const
        {
            return Find(target.m_Data) != npos;
        }

        /**
         * @brief Returns true if the view contains the given character.
         *
         * @param ch The character to check for.
         * @return True if the character is found, false otherwise.
         */
        bool Contains(char ch) const
        {
            return Find(ch) != npos;
        }

        /**
         * @brief Compare two StringViews for equality.
         *
         * @param other The other StringView to compare with.
         * @return True if equal, false otherwise.
         */
        bool operator==(const StringView& other) const
        {
            return m_Length == other.m_Length && std::strncmp(m_Data, other.m_Data, m_Length) == 0;
        }

        /**
         * @brief Compare two StringViews for inequality.
         *
         * @param other The other StringView to compare with.
         * @return True if not equal, false otherwise.
         */
        bool operator!=(const StringView& other) const { return !(*this == other); }

        const char* begin() const { return m_Data; }
        const char* end() const { return m_Data + m_Length; }

        static constexpr uint64_t npos = static_cast<uint64_t>(-1);

    private:
        const char* m_Data;
        uint64_t m_Length;
        static constexpr char s_NullChar = '\0';
    };
}
