#pragma once

#include <memory>

namespace HBL2
{
    struct ByteSpan
    {
        constexpr ByteSpan() noexcept = default;

        ByteSpan(void* bytes, size_t byte_size) noexcept
            : m_Data(static_cast<std::byte*>(bytes)), m_Size(byte_size) {}

        template<typename T> requires(std::is_class_v<T> && !std::is_volatile_v<T>&& std::is_trivially_copyable_v<T> && (sizeof(T) & 3u) == 0)
        ByteSpan(const T& value) noexcept
            : m_Data(reinterpret_cast<std::byte*>(std::addressof(value))), m_Size(sizeof(T)) {}

        void set_data(void* bytes, size_t byte_size)
        {
            m_Data = static_cast<std::byte*>(bytes);
            m_Size = byte_size;
        }

        const std::byte* data() const { return m_Data; }
        const size_t size() const { return m_Size; }

        std::byte* data() { return m_Data; }
        size_t size() { return m_Size; }

        const std::byte* begin() const { return m_Data; }
        const std::byte* end() const { return m_Data + m_Size; }

        std::byte* begin() { return m_Data; }
        std::byte* end() { return m_Data + m_Size; }

        operator bool() const
        {
            return (bool)m_Data;
        }

        std::byte& operator[](int index)
        {
            return m_Data[index];
        }

        std::byte operator[](int index) const
        {
            return m_Data[index];
        }

        template<typename T>
        T* as() const
        {
            return (T*)m_Data;
        }

    private:
        std::byte* m_Data = nullptr;
        size_t m_Size = 0;
    };
}