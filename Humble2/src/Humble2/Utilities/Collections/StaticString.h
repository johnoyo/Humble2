#pragma once

#include <spdlog/fmt/fmt.h>

#include <string_view>
#include <cstring>
#include <algorithm>

namespace HBL2
{
    template <size_t Capacity>
    class StaticString
    {
    public:
        static constexpr size_t kCapacity = Capacity;

        constexpr StaticString() noexcept = default;

        constexpr StaticString(std::string_view str) noexcept
        {
            assign(str);
        }

        constexpr StaticString(const char* str) noexcept
        {
            assign(std::string_view(str, str ? std::char_traits<char>::length(str) : 0));
        }

        StaticString(const std::string& str) noexcept
        {
            assign(std::string_view(str));
        }

        constexpr void clear() noexcept
        {
            m_Size = 0;
            m_Data[0] = '\0';
        }

        constexpr size_t size() const noexcept { return m_Size; }
        constexpr size_t capacity() const noexcept { return Capacity - 1; } // reserve space for null
        constexpr bool empty() const noexcept { return m_Size == 0; }

        constexpr const char* data() const noexcept { return m_Data; }
        constexpr const char* c_str() const noexcept { return m_Data; }

        constexpr std::string_view view() const noexcept
        {
            return std::string_view(m_Data, m_Size);
        }

        constexpr void assign(std::string_view str) noexcept
        {
            const size_t len = std::min(str.size(), capacity());
            if (len > 0)
            {
                std::memcpy(m_Data, str.data(), len);
            }

            m_Data[len] = '\0';
            m_Size = static_cast<uint16_t>(len);
        }

        constexpr void assign(const char* str) noexcept
        {
            assign(std::string_view(str, str ? std::char_traits<char>::length(str) : 0));
        }

        void assign(const std::string& str) noexcept
        {
            assign(std::string_view(str));
        }

        constexpr void append(std::string_view str) noexcept
        {
            const size_t remaining = capacity() - m_Size;
            const size_t len = std::min(str.size(), remaining);

            if (len > 0)
            {
                std::memcpy(m_Data + m_Size, str.data(), len);
            }

            m_Size += static_cast<uint16_t>(len);
            m_Data[m_Size] = '\0';
        }

        constexpr void append(const char* str) noexcept
        {
            append(std::string_view(str, str ? std::char_traits<char>::length(str) : 0));
        }

        void append(const std::string& str) noexcept
        {
            append(std::string_view(str));
        }

        static constexpr size_t npos = size_t(-1);

        [[nodiscard]] constexpr size_t find(std::string_view str, size_t pos = 0) const noexcept
        {
            if (str.empty())
            {
                return pos;
            }

            if (pos >= m_Size || str.size() > m_Size - pos)
            {
                return npos;
            }

            const size_t end = m_Size - str.size();
            for (size_t i = pos; i <= end; ++i)
            {
                if (std::string_view(m_Data + i, str.size()) == str)
                {
                    return i;
                }
            }

            return npos;
        }

        [[nodiscard]] constexpr size_t find(const char* str, size_t pos = 0) const noexcept
        {
            return find(std::string_view(str, str ? std::char_traits<char>::length(str) : 0), pos);
        }

        [[nodiscard]] size_t find(const std::string& str, size_t pos = 0) const noexcept
        {
            return find(std::string_view(str), pos);
        }

        [[nodiscard]] constexpr size_t find(char c, size_t pos = 0) const noexcept
        {
            for (size_t i = pos; i < m_Size; ++i)
            {
                if (m_Data[i] == c)
                {
                    return i;
                }
            }

            return npos;
        }

        template<typename... Args>
        void format(fmt::format_string<Args...> fmtStr, Args&&... args)
        {
            auto result = fmt::format_to_n(m_Data, capacity(), fmtStr, std::forward<Args>(args)...);

            m_Size = static_cast<uint16_t>(std::min(result.size, capacity()));
            m_Data[m_Size] = '\0';
        }

        constexpr char operator[](size_t i) const noexcept
        {
            return m_Data[i];
        }

        constexpr bool operator==(std::string_view other) const noexcept { return view() == other; }
        constexpr bool operator==(const char* other) const noexcept { return view() == std::string_view(other); }
        bool operator==(const std::string& other) const noexcept { return view() == std::string_view(other); }

        constexpr bool operator!=(std::string_view other) const noexcept { return !(*this == other); }
        constexpr bool operator!=(const char* other) const noexcept { return !(*this == other); }
        bool operator!=(const std::string& other) const noexcept { return !(*this == other); }

        constexpr StaticString<Capacity>& operator+=(std::string_view str) noexcept { append(str); return *this; }
        constexpr StaticString<Capacity>& operator+=(const char* str) noexcept { append(str); return *this; }
        StaticString<Capacity>& operator+=(const std::string& str) noexcept { append(str); return *this; }
        constexpr StaticString<Capacity>& operator+=(char c) noexcept
        {
            if (m_Size < capacity())
            {
                m_Data[m_Size++] = c;
                m_Data[m_Size] = '\0';
            }
            return *this;
        }

        [[nodiscard]] constexpr StaticString<Capacity> operator+(std::string_view str) const noexcept
        {
            StaticString<Capacity> result(*this);
            result.append(str);
            return result;
        }

        [[nodiscard]] constexpr StaticString<Capacity> operator+(const char* str) const noexcept
        {
            return *this + std::string_view(str, str ? std::char_traits<char>::length(str) : 0);
        }

        [[nodiscard]] StaticString<Capacity> operator+(const std::string& str) const noexcept
        {
            StaticString<Capacity> result(*this);
            result.append(std::string_view(str));
            return result;
        }

        [[nodiscard]] constexpr StaticString<Capacity> operator+(char c) const noexcept
        {
            StaticString<Capacity> result(*this);
            result += c;
            return result;
        }

    private:
        char m_Data[Capacity]{};
        uint16_t m_Size = 0;
    };
}