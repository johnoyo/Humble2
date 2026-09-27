#pragma once

#include <memory>

namespace HBL2
{
    struct ByteSpan
    {
        constexpr ByteSpan() noexcept = default;

        ByteSpan(const void* bytes, size_t byte_size) noexcept
            : m_Data(static_cast<const std::byte*>(bytes)), m_Size(byte_size) {}

        template<typename T> requires(std::is_class_v<T> && !std::is_volatile_v<T>&& std::is_trivially_copyable_v<T> && (sizeof(T) & 3u) == 0)
        ByteSpan(const T& value) noexcept
            : m_Data(reinterpret_cast<const std::byte*>(std::addressof(value))), m_Size(sizeof(T)) {}

        const std::byte* data() const { return m_Data; }
        const size_t size() const { return m_Size; }

        const std::byte* begin() const { return m_Data; }
        const std::byte* end() const { return m_Data + m_Size; }

    private:
        const std::byte* m_Data = nullptr;
        size_t m_Size = 0;
    };
}