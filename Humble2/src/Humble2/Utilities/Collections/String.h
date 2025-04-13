#pragma once

#include <Utilities\Allocators\StandardAllocator.h>

#include <cstring>
#include <cstdarg>

namespace HBL2
{
    /// <summary>
    /// A string class that uses Small String Optimization (SSO) for small strings
    /// and dynamically allocates memory for larger strings.
    /// </summary>
    /// <typeparam name="TAllocator">The allocator type to use for memory management.</typeparam>
    template<typename TAllocator = StandardAllocator>
    class String
    {
    public:
        /// <summary>
        /// The size threshold for Small String Optimization.
        /// Strings smaller than this size will be stored inline.
        /// </summary>
        static constexpr size_t SSO_SIZE = 16;

        /// <summary>
        /// Default constructor. Creates an empty string.
        /// </summary>
        String() : m_Size(0), m_Capacity(SSO_SIZE), m_HeapData(nullptr), m_Allocator(nullptr)
        {
            m_SSOBuffer[0] = '\0';
            m_IsUsingSSO = true;
        }

        /// <summary>
        /// Constructor with a provided allocator.
        /// </summary>
        /// <param name="allocator">The allocator to use.</param>
        String(TAllocator* allocator) : m_Size(0), m_Capacity(SSO_SIZE), m_HeapData(nullptr), m_Allocator(allocator)
        {
            m_SSOBuffer[0] = '\0';
            m_IsUsingSSO = true;
        }

        /// <summary>
        /// Constructor from a C-style string.
        /// </summary>
        /// <param name="str">The C-style string to copy.</param>
        String(const char* str) : m_Allocator(nullptr)
        {
            size_t len = std::strlen(str);
            if (len < SSO_SIZE)
            {
                std::memcpy(m_SSOBuffer, str, len + 1);
                m_Size = len;
                m_Capacity = SSO_SIZE;
                m_IsUsingSSO = true;
            }
            else
            {
                m_Capacity = m_Size * 2;
                m_HeapData = m_Allocator->Allocate<char>(len + 1);
                std::memcpy(m_HeapData, str, len + 1);
                m_Size = len;
                m_IsUsingSSO = false;
            }
        }

        /// <summary>
        /// Constructor from a C-style string with a provided allocator.
        /// </summary>
        /// <param name="allocator">The allocator to use.</param>
        /// <param name="str">The C-style string to copy.</param>
        String(TAllocator* allocator, const char* str) : m_Allocator(allocator)
        {
            size_t len = std::strlen(str);
            if (len < SSO_SIZE)
            {
                std::memcpy(m_SSOBuffer, str, len + 1);
                m_Size = len;
                m_Capacity = SSO_SIZE;
                m_IsUsingSSO = true;
            }
            else
            {
                m_Capacity = m_Size * 2;
                m_HeapData = m_Allocator->Allocate<char>(len + 1);
                std::memcpy(m_HeapData, str, len + 1);
                m_Size = len;
                m_IsUsingSSO = false;
            }
        }

        /// <summary>
        /// Copy constructor.
        /// </summary>
        /// <param name="other">The string to copy from.</param>
        String(const String& other) : m_Allocator(other.m_Allocator) { CopyFrom(other); }

        /// <summary>
        /// Move constructor.
        /// </summary>
        /// <param name="other">The string to move from.</param>
        String(String&& other) noexcept
            : m_Size(other.m_Size), m_Capacity(other.m_Capacity), m_IsUsingSSO(other.m_IsUsingSSO), m_Allocator(other.m_Allocator)
        {
            if (m_IsUsingSSO)
            {
                std::memcpy(m_SSOBuffer, other.m_SSOBuffer, m_Size + 1);
            }
            else
            {
                m_HeapData = other.m_HeapData;
                other.m_HeapData = nullptr;
            }

            other.m_Size = 0;
            other.m_Capacity = SSO_SIZE;
            other.m_IsUsingSSO = true;
            other.m_SSOBuffer[0] = '\0';
        }

        /// <summary>
        /// Destructor.
        /// </summary>
        ~String() { Clear(); }

        /// <summary>
        /// Copy assignment operator.
        /// </summary>
        /// <param name="other">The string to copy from.</param>
        /// <returns>Reference to this string.</returns>
        String& operator=(const String& other)
        {
            if (this != &other)
            {
                Clear();
                CopyFrom(other);
            }
            return *this;
        }

        /// <summary>
        /// Move assignment operator.
        /// </summary>
        /// <param name="other">The string to move from.</param>
        /// <returns>Reference to this string.</returns>
        String& operator=(String&& other) noexcept
        {
            if (this != &other)
            {
                Clear();

                m_Size = other.m_Size;
                m_Capacity = other.m_Capacity;
                m_IsUsingSSO = other.m_IsUsingSSO;
				m_Allocator = other.m_Allocator;

                if (m_IsUsingSSO)
                {
                    std::memcpy(m_SSOBuffer, other.m_SSOBuffer, m_Size + 1);
                }
                else
                {
                    m_HeapData = other.m_HeapData;
                    other.m_HeapData = nullptr;
                }

                other.m_Size = 0;
                other.m_Capacity = SSO_SIZE;
                other.m_IsUsingSSO = true;
                other.m_SSOBuffer[0] = '\0';
				other.m_Allocator = nullptr
            }
            return *this;
        }

        /// <summary>
        /// Returns the underlining c style char pointer.
        /// </summary>
        /// <returns>The underlining c style char pointer.</returns>
        const char* Data() const { return m_IsUsingSSO ? m_SSOBuffer : m_HeapData; }

        /// <summary>
        /// Returns the number of characters in the string.
        /// </summary>
        /// <returns>The number of characters in the string.</returns>
        size_t Length() const { return m_Size; }

        /// <summary>
        /// Returns the current capacity of the string.
        /// </summary>
        /// <returns>The current capacity of the string.</returns>
        size_t Capacity() const { return m_Capacity; }

        /// <summary>
        /// Returns true if the string is empty, false otherwise.
        /// </summary>
        /// <returns>True if the string is empty, false otherwise.</returns>
        bool Empty() const { return m_Size == 0; }

        /// <summary>
        /// Compares two strings for equality.
        /// </summary>
        /// <param name="other">The string to compare with.</param>
        /// <returns>True if the strings are equal, false otherwise.</returns>
        bool operator==(const String& other) const
        {
            if (m_Size != other.m_Size)
            {
                return false;
            }

            return std::strcmp(Data(), other.Data()) == 0;
        }

        /// <summary>
        /// Compares two strings for inequality.
        /// </summary>
        /// <param name="other">The string to compare with.</param>
        /// <returns>True if the strings are not equal, false otherwise.</returns>
        bool operator!=(const String& other) const
        {
            return !(*this == other);
        }

        /// <summary>
        /// Access character at specified index.
        /// </summary>
        /// <param name="index">The index of the character to access.</param>
        /// <returns>Reference to the character at the specified index.</returns>
        char& operator[](size_t index)
        {
            return const_cast<char&>(static_cast<const String&>(*this)[index]);
        }

        /// <summary>
        /// Access character at specified index (const version).
        /// </summary>
        /// <param name="index">The index of the character to access.</param>
        /// <returns>Reference to the character at the specified index.</returns>
        const char& operator[](size_t index) const
        {
            if (index >= m_Size)
            {
                HBL2_CORE_ERROR("String index out of range, returning empty character.");
                return '\0';
            }

            return Data()[index];
        }

        /// <summary>
        /// Create a new sub string.
        /// </summary>
        /// <param name="start">The start index in the string.</param>
        /// <param name="length">The end index in the string.</param>
        /// <returns>A new sub string.</returns>
        String<TAllocator> SubString(uint32_t start, uint32_t length) const
        {
            if (start >= m_Size)
            {
                return String<TAllocator>(m_Allocator, "");
            }

            size_t maxLength = (start + length > m_Size) ? m_Size - start : length;

            // Create a temporary buffer for the substring
            char* temp = Allocate(maxLength + 1);
            std::memcpy(temp, Data() + start, maxLength);
            temp[maxLength] = '\0';

            // Create a new string from the temporary buffer
            String<TAllocator> result(m_Allocator, temp);
            Deallocate(temp);

            return result;
        }

        /// <summary>
        /// Find a substring
        /// </summary>
        /// <param name="substr">The substring to find.</param>
        /// <returns>True if the substring exists in the string, false otherwise.</returns>
        bool Find(const char* substr) const
        {
            const char* found = std::strstr(Data(), substr);
            return found;
        }

        /// <summary>
        /// Append a C-style string to this string.
        /// </summary>
        /// <param name="str">The C-style string to append.</param>
        /// <returns>Reference to this string.</returns>
        String& Append(const char* str)
        {
            size_t strLen = std::strlen(str);
			if (strLen == 0)
			{
				return *this;
			}

            size_t newSize = m_Size + strLen;
            if (newSize > m_Capacity)
            {
                Reserve(newSize * 2);
            }

            char* dest = const_cast<char*>(Data()) + m_Size;
            std::memcpy(dest, str, strLen + 1);
            m_Size = newSize;

            return *this;
        }

        /// <summary>
        /// Append another string to this string.
        /// </summary>
        /// <param name="other">The string to append.</param>
        /// <returns>Reference to this string.</returns>
        String& Append(const String& other)
        {
            return Append(other.Data());
        }

        /// <summary>
        /// Concatenate two strings.
        /// </summary>
        /// <param name="other">The string to concatenate.</param>
        /// <returns>A new string containing the concatenation.</returns>
        String operator+(const String& other) const
        {
            String result(*this);
            result.Append(other);
            return result;
        }

        /// <summary>
        /// Append a string to this string.
        /// </summary>
        /// <param name="other">The string to append.</param>
        /// <returns>Reference to this string.</returns>
        String& operator+=(const String& other)
        {
            return Append(other);
        }

        /// <summary>
        /// Reserve memory for the string.
        /// </summary>
        /// <param name="capacity">The new capacity to reserve.</param>
        void Reserve(size_t capacity)
        {
            if (capacity <= m_Capacity)
            {
                return;
            }

            char* newBuffer = Allocate(capacity + 1);
            std::memcpy(newBuffer, Data(), m_Size + 1);

            if (!m_IsUsingSSO)
            {
                Deallocate(m_HeapData);
            }

            m_HeapData = newBuffer;
            m_Capacity = capacity;
            m_IsUsingSSO = false;
        }

        /// <summary>
        /// Clear the string.
        /// </summary>
        void Clear()
        {
            if (!m_IsUsingSSO && m_HeapData != nullptr)
            {
                Deallocate(m_HeapData);
                m_HeapData = nullptr;
            }

            m_Size = 0;
            m_Capacity = SSO_SIZE;
            m_IsUsingSSO = true;
            m_SSOBuffer[0] = '\0';
        }

    private:
        void CopyFrom(const String& other)
        {
            m_Size = other.m_Size;
            m_Capacity = other.m_Capacity;
            m_Allocator = other.m_Allocator;

            if (!other.m_IsUsingSSO)
            {
                m_HeapData = Allocate(m_Size + 1);
                std::memcpy(m_HeapData, other.m_HeapData, m_Size + 1);
                m_IsUsingSSO = false;
            }
            else
            {
                std::memcpy(m_SSOBuffer, other.m_SSOBuffer, m_Size + 1);
                m_IsUsingSSO = true;
            }
        }

    private:
		char* Allocate(uint64_t size)
		{
			if (m_Allocator == nullptr)
			{
				char* data = (char*)operator new(size);
				memset(data, 0, size);				
				return data;
			}

			return m_Allocator->Allocate<char>(size);
		}

		void Deallocate(char* ptr)
		{
			if (m_Allocator == nullptr)
			{
				delete ptr;
				return;
			}

			m_Allocator->Deallocate<char>(ptr);
		}

    private:
        uint32_t m_Size; // Not in bytes
        uint32_t m_Capacity; // Not in bytes
        union
        {
            char m_SSOBuffer[SSO_SIZE + 1];
            char* m_HeapData;
        };
        bool m_IsUsingSSO = true;

        TAllocator* m_Allocator = nullptr; // Does not own the pointer
    };

    template<typename TAllocator>
    auto MakeString(TAllocator* allocator)
    {
        return String<TAllocator>(allocator);
    }

    template<typename TAllocator>
    auto MakeString(TAllocator* allocator, const char* str)
    {
        return String<TAllocator>(allocator, str);
    }
}