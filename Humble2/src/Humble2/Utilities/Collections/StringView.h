#pragma once

#include "String.h"

#include <cstddef>
#include <cstring>
#include <string>
#include <algorithm>

namespace HBL2
{
    /// <summary>
    /// A lightweight, non-owning view of a contiguous character sequence, similar to std::string_view.
    /// It provides read-only access to string data without copying, making it ideal for efficient string manipulation and function parameter passing.
    /// </summary>
    class StringView
    {
    public:
        /// <summary>
        /// Default constructor creates an empty view.
        /// </summary>
        constexpr StringView() noexcept : m_Data(""), m_Length(0) {}

        /// <summary>
        /// Constructor from a null-terminated C-style string.
        /// </summary>
        StringView(const char* str)
            : m_Data(str), m_Length(str ? std::strlen(str) : 0) {}

        /// <summary>
        /// Constructor from a char pointer and length.
        /// </summary>
        constexpr StringView(const char* str, uint64_t len)
            : m_Data(str), m_Length(len) {}

        /// <summary>
        /// Constructor from std::string.
        /// </summary>
        StringView(const std::string& str)
            : m_Data(str.data()), m_Length(str.length()) {}

        /// <summary>
        /// Constructor from your custom String class.
        /// </summary>
        template<typename TAllocator>
        StringView(const String<TAllocator>& str)
            : m_Data(str.Data()), m_Length(str.Length()) {}

        /// <summary>
        /// Returns a pointer to the underlying data.
        /// </summary>
        constexpr const char* Data() const noexcept { return m_Data; }

        /// <summary>
        /// Returns the number of characters in the view.
        /// </summary>
        constexpr uint64_t Length() const noexcept { return m_Length; }

        /// <summary>
        /// Returns true if the view is empty.
        /// </summary>
        constexpr bool Empty() const noexcept { return m_Length == 0; }

        /// <summary>
        /// Converts to std::string.
        /// </summary>
        std::string ToStdString() const { return std::string(m_Data, m_Length); }

        /// <summary>
        /// Converts the view to a full String using a provided allocator.
        /// </summary>
        template<typename TAllocator = StandardAllocator>
        String<TAllocator> ToString(TAllocator* allocator = nullptr) const
        {
            return String<TAllocator>(allocator, m_Data, m_Length);
        }

        /// <summary>
        /// Access character at the specified index.
        /// </summary>
        constexpr const char& operator[](uint64_t index) const
        {
            return (index < m_Length) ? m_Data[index] : s_NullChar;
        }

        /// <summary>
        /// Front character.
        /// </summary>
        constexpr char Front() const { return m_Length > 0 ? m_Data[0] : s_NullChar; }

        /// <summary>
        /// Back character.
        /// </summary>
        constexpr char Back() const { return m_Length > 0 ? m_Data[m_Length - 1] : s_NullChar; }

        /// <summary>
        /// Remove prefix of N characters.
        /// </summary>
        void RemovePrefix(uint64_t n)
        {
            n = std::min(n, m_Length);
            m_Data += n;
            m_Length -= n;
        }

        /// <summary>
        /// Remove suffix of N characters.
        /// </summary>
        void RemoveSuffix(uint64_t n)
        {
            m_Length = (n >= m_Length) ? 0 : m_Length - n;
        }

        /// <summary>
        /// Create a substring view from this view.
        /// </summary>
        StringView SubString(uint64_t start, uint64_t count = npos) const
        {
            if (start >= m_Length)
            {
                return {};
            }

            uint64_t len = std::min(count, m_Length - start);
            return StringView(m_Data + start, len);
        }

        /// <summary>
        /// Find substring in view.
        /// </summary>
        uint64_t Find(const char* substr, uint64_t pos = 0) const
        {
            if (pos >= m_Length || substr == nullptr)
            {
                return npos;
            }

            const char* found = std::strstr(m_Data + pos, substr);
            return (found && found < m_Data + m_Length) ? static_cast<uint64_t>(found - m_Data) : npos;
        }

        /// <summary>
        /// Find first occurrence of a character.
        /// </summary>
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

        /// <summary>
        /// Returns true if the view starts with the given string or character.
        /// </summary>
        bool StartsWith(const StringView& prefix) const
        {
            if (prefix.m_Length > m_Length)
            {
                return false;
            }

            return std::strncmp(m_Data, prefix.m_Data, prefix.m_Length) == 0;
        }

        bool StartsWith(char ch) const
        {
            return m_Length > 0 && m_Data[0] == ch;
        }

        /// <summary>
        /// Returns true if the view ends with the given string or character.
        /// </summary>
        bool EndsWith(const StringView& suffix) const
        {
            if (suffix.m_Length > m_Length)
            {
                return false;
            }

            return std::strncmp(m_Data + m_Length - suffix.m_Length, suffix.m_Data, suffix.m_Length) == 0;
        }

        bool EndsWith(char ch) const
        {
            return m_Length > 0 && m_Data[m_Length - 1] == ch;
        }

        /// <summary>
        /// Returns true if the view contains the given substring.
        /// </summary>
        bool Contains(const StringView& target) const
        {
            return Find(target.m_Data) != npos;
        }

        bool Contains(char ch) const
        {
            return Find(ch) != npos;
        }

        /// <summary>
        /// Compare two StringViews.
        /// </summary>
        bool operator==(const StringView& other) const
        {
            return m_Length == other.m_Length && std::strncmp(m_Data, other.m_Data, m_Length) == 0;
        }

        bool operator!=(const StringView& other) const { return !(*this == other); }

        /// <summary>
        /// Begin iterator.
        /// </summary>
        const char* begin() const { return m_Data; }

        /// <summary>
        /// End iterator.
        /// </summary>
        const char* end() const { return m_Data + m_Length; }

        static constexpr uint64_t npos = static_cast<uint64_t>(-1);

    private:
        const char* m_Data;
        uint64_t m_Length;
        static constexpr char s_NullChar = '\0';
    };
}
