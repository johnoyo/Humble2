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

    private:
        char m_Data[Capacity]{};
        uint16_t m_Size = 0;
    };
}