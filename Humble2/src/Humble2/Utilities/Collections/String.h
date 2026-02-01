#pragma once

#include <Utilities\Allocators\StandardAllocator.h>

#include <cstring>
#include <cstdarg>

namespace HBL2
{
    /**
     * @brief A string class that uses Small String Optimization (SSO) for small strings and dynamically allocates memory for larger strings.
     *
     * @tparam TAllocator The allocator type to use for memory management.
     */
  //  template<typename TAllocator = StandardAllocator>
  //  class String
  //  {
  //  public:
  //      /**
  //       * @brief The size threshold for Small String Optimization.
  //       *
  //       * Strings smaller than this size will be stored inline.
  //       */
  //      static constexpr size_t SSO_SIZE = 16;

  //      /**
  //       * @brief Default constructor. Creates an empty string.
  //       */
  //      String()
  //          : m_Size(0), m_Capacity(SSO_SIZE), m_HeapData(nullptr), m_Allocator(nullptr)
  //      {
  //          m_SSOBuffer[0] = '\0';
  //          m_IsUsingSSO = true;
  //      }

  //      /**
  //       * @brief Constructor with a provided allocator.
  //       *
  //       * @param allocator The allocator to use.
  //       */
  //      String(TAllocator* allocator)
  //          : m_Size(0), m_Capacity(SSO_SIZE), m_HeapData(nullptr), m_Allocator(allocator)
  //      {
  //          m_SSOBuffer[0] = '\0';
  //          m_IsUsingSSO = true;
  //      }

  //      /**
  //       * @brief Constructor from a C-style string.
  //       *
  //       * @param str The C-style string to copy.
  //       */
  //      String(const char* str)
  //          : m_Allocator(nullptr)
  //      {
  //          size_t len = std::strlen(str);
  //          if (len < SSO_SIZE)
  //          {
  //              std::memcpy(m_SSOBuffer, str, len + 1);
  //              m_Size = len;
  //              m_Capacity = SSO_SIZE;
  //              m_IsUsingSSO = true;
  //          }
  //          else
  //          {
  //              m_Size = len;
  //              m_Capacity = m_Size * 2;
  //              m_HeapData = Allocate(len + 1);
  //              std::memcpy(m_HeapData, str, len + 1);
  //              m_IsUsingSSO = false;
  //          }
  //      }

  //      /**
  //       * @brief Constructor from a C-style string with a provided allocator.
  //       *
  //       * @param allocator The allocator to use.
  //       * @param str The C-style string to copy.
  //       */
  //      String(TAllocator* allocator, const char* str)
  //          : m_Allocator(allocator)
  //      {
  //          size_t len = std::strlen(str);
  //          if (len < SSO_SIZE)
  //          {
  //              std::memcpy(m_SSOBuffer, str, len + 1);
  //              m_Size = len;
  //              m_Capacity = SSO_SIZE;
  //              m_IsUsingSSO = true;
  //          }
  //          else
  //          {
  //              m_Size = len;
  //              m_Capacity = m_Size * 2;
  //              m_HeapData = Allocate(len + 1);
  //              std::memcpy(m_HeapData, str, len + 1);
  //              m_IsUsingSSO = false;
  //          }
  //      }

  //      /**
  //       * @brief Constructor from a C-style string and length with a provided allocator.
  //       *
  //       * @param allocator The allocator to use.
  //       * @param str The C-style string to copy.
  //       * @param len The string length.
  //       */
  //      String(TAllocator* allocator, const char* str, size_t len)
  //          : m_Allocator(allocator)
  //      {
  //          if (len < SSO_SIZE)
  //          {
  //              std::memcpy(m_SSOBuffer, str, len);
  //              m_SSOBuffer[len] = '\0';
  //              m_Size = static_cast<uint32_t>(len);
  //              m_Capacity = SSO_SIZE;
  //              m_IsUsingSSO = true;
  //          }
  //          else
  //          {
  //              m_Capacity = static_cast<uint32_t>(len * 2);
  //              m_HeapData = Allocate(m_Capacity + 1);
  //              std::memcpy(m_HeapData, str, len);
  //              m_HeapData[len] = '\0';
  //              m_Size = static_cast<uint32_t>(len);
  //              m_IsUsingSSO = false;
  //          }
  //      }

  //      /**
  //       * @brief Copy constructor.
  //       *
  //       * @param other The string to copy from.
  //       */
  //      String(const String& other)
  //          : m_Allocator(other.m_Allocator)
  //      {
  //          CopyFrom(other);
  //      }

  //      /**
  //       * @brief Move constructor.
  //       *
  //       * @param other The string to move from.
  //       */
  //      String(String&& other) noexcept
  //          : m_Size(other.m_Size), m_Capacity(other.m_Capacity), m_IsUsingSSO(other.m_IsUsingSSO), m_Allocator(other.m_Allocator)
  //      {
  //          if (m_IsUsingSSO)
  //          {
  //              std::memcpy(m_SSOBuffer, other.m_SSOBuffer, m_Size + 1);
  //          }
  //          else
  //          {
  //              m_HeapData = other.m_HeapData;
  //              other.m_HeapData = nullptr;
  //          }

  //          other.m_Size = 0;
  //          other.m_Capacity = SSO_SIZE;
  //          other.m_IsUsingSSO = true;
  //          other.m_SSOBuffer[0] = '\0';
  //      }

  //      /**
  //       * @brief Destructor.
  //       */
  //      ~String() { Clear(); }

  //      /**
  //       * @brief Copy assignment operator.
  //       *
  //       * @param other The string to copy from.
  //       * @return Reference to this string.
  //       */
  //      String& operator=(const String& other)
  //      {
  //          if (this != &other)
  //          {
  //              Clear();
  //              CopyFrom(other);
  //          }
  //          return *this;
  //      }

  //      /**
  //       * @brief Move assignment operator.
  //       *
  //       * @param other The string to move from.
  //       * @return Reference to this string.
  //       */
  //      String& operator=(String&& other) noexcept
  //      {
  //          if (this != &other)
  //          {
  //              Clear();

  //              m_Size = other.m_Size;
  //              m_Capacity = other.m_Capacity;
  //              m_IsUsingSSO = other.m_IsUsingSSO;
		//		m_Allocator = other.m_Allocator;

  //              if (m_IsUsingSSO)
  //              {
  //                  std::memcpy(m_SSOBuffer, other.m_SSOBuffer, m_Size + 1);
  //              }
  //              else
  //              {
  //                  m_HeapData = other.m_HeapData;
  //                  other.m_HeapData = nullptr;
  //              }

  //              other.m_Size = 0;
  //              other.m_Capacity = SSO_SIZE;
  //              other.m_IsUsingSSO = true;
  //              other.m_SSOBuffer[0] = '\0';
  //              other.m_Allocator = nullptr;
  //          }
  //          return *this;
  //      }

  //      /**
  //       * @brief Returns the underlying C-style char pointer.
  //       *
  //       * @return The underlying C-style char pointer.
  //       */
  //      const char* Data() const { return m_IsUsingSSO ? m_SSOBuffer : m_HeapData; }

  //      /**
  //       * @brief Returns the number of characters in the string.
  //       *
  //       * @return The number of characters in the string.
  //       */
  //      size_t Length() const { return m_Size; }

  //      /**
  //       * @brief Returns the current capacity of the string.
  //       *
  //       * @return The current capacity of the string.
  //       */
  //      size_t Capacity() const { return m_Capacity; }

  //      /**
  //       * @brief Returns true if the string is empty, false otherwise.
  //       *
  //       * @return True if the string is empty, false otherwise.
  //       */
  //      bool Empty() const { return m_Size == 0; }

  //      /**
  //       * @brief Compares two strings for equality.
  //       *
  //       * @param other The string to compare with.
  //       * @return True if the strings are equal, false otherwise.
  //       */
  //      bool operator==(const String& other) const
  //      {
  //          if (m_Size != other.m_Size)
  //          {
  //              return false;
  //          }

  //          return std::strcmp(Data(), other.Data()) == 0;
  //      }

  //      /**
  //       * @brief Compares two strings for inequality.
  //       *
  //       * @param other The string to compare with.
  //       * @return True if the strings are not equal, false otherwise.
  //       */
  //      bool operator!=(const String& other) const
  //      {
  //          return !(*this == other);
  //      }

  //      /**
  //       * @brief Access character at specified index.
  //       *
  //       * @param index The index of the character to access.
  //       * @return Reference to the character at the specified index.
  //       */
  //      char& operator[](size_t index)
  //      {
  //          return const_cast<char&>(static_cast<const String&>(*this)[index]);
  //      }

  //      /**
  //       * @brief Access character at specified index (const version).
  //       *
  //       * @param index The index of the character to access.
  //       * @return Reference to the character at the specified index.
  //       */
  //      const char& operator[](size_t index) const
  //      {
  //          if (index >= m_Size)
  //          {
  //              HBL2_CORE_ERROR("String index out of range, returning empty character.");
  //              return '\0';
  //          }

  //          return Data()[index];
  //      }

  //      /**
  //       * @brief Create a new substring.
  //       *
  //       * @param start The start index in the string.
  //       * @param length The length of the substring.
  //       * @return A new substring.
  //       */
  //      String<TAllocator> SubString(uint32_t start, uint32_t length) const
  //      {
  //          if (start >= m_Size)
  //          {
  //              return String<TAllocator>(m_Allocator, "");
  //          }

  //          size_t maxLength = (start + length > m_Size) ? m_Size - start : length;

  //          // Create a temporary buffer for the substring
  //          char* temp = Allocate(maxLength + 1);
  //          std::memcpy(temp, Data() + start, maxLength);
  //          temp[maxLength] = '\0';

  //          // Create a new string from the temporary buffer
  //          String<TAllocator> result(m_Allocator, temp);
  //          Deallocate(temp);

  //          return result;
  //      }

  //      /**
  //       * @brief Find a substring.
  //       *
  //       * @param substr The substring to find.
  //       * @return True if the substring exists in the string, false otherwise.
  //       */
  //      bool Find(const char* substr) const
  //      {
  //          const char* found = std::strstr(Data(), substr);
  //          return found;
  //      }

  //      /**
  //       * @brief Append a C-style string to this string.
  //       *
  //       * @param str The C-style string to append.
  //       * @return Reference to this string.
  //       */
  //      String& Append(const char* str)
  //      {
  //          size_t strLen = std::strlen(str);
		//	if (strLen == 0)
		//	{
		//		return *this;
		//	}

  //          size_t newSize = m_Size + strLen;
  //          if (newSize > m_Capacity)
  //          {
  //              Reserve(newSize * 2);
  //          }

  //          char* dest = const_cast<char*>(Data()) + m_Size;
  //          std::memcpy(dest, str, strLen + 1);
  //          m_Size = newSize;

  //          return *this;
  //      }

  //      /**
  //       * @brief Append another string to this string.
  //       *
  //       * @param other The string to append.
  //       * @return Reference to this string.
  //       */
  //      String& Append(const String& other)
  //      {
  //          size_t strLen = other.m_Size;
  //          if (strLen == 0)
  //          {
  //              return *this;
  //          }

  //          size_t newSize = m_Size + strLen;
  //          if (newSize > m_Capacity)
  //          {
  //              Reserve(newSize * 2);
  //          }

  //          char* dest = const_cast<char*>(Data()) + m_Size;
  //          std::memcpy(dest, other.Data(), strLen + 1);
  //          m_Size = newSize;

  //          return *this;
  //      }

  //      /**
  //       * @brief Concatenate two strings.
  //       *
  //       * @param other The string to concatenate.
  //       * @return A new string containing the concatenation.
  //       */
  //      String operator+(const String& other) const
  //      {
  //          String result(*this);
  //          result.Append(other);
  //          return result;
  //      }

  //      /**
  //       * @brief Append a string to this string.
  //       *
  //       * @param other The string to append.
  //       * @return Reference to this string.
  //       */
  //      String& operator+=(const String& other)
  //      {
  //          return Append(other);
  //      }

  //      /**
  //       * @brief Reserve memory for the string.
  //       *
  //       * @param capacity The new capacity to reserve.
  //       */
  //      void Reserve(size_t capacity)
  //      {
  //          if (capacity <= m_Capacity)
  //          {
  //              return;
  //          }

  //          char* newBuffer = Allocate(capacity + 1);
  //          std::memcpy(newBuffer, Data(), m_Size + 1);

  //          if (!m_IsUsingSSO)
  //          {
  //              Deallocate(m_HeapData);
  //          }

  //          m_HeapData = newBuffer;
  //          m_Capacity = capacity;
  //          m_IsUsingSSO = false;
  //      }

  //      /**
  //       * @brief Clear the string.
  //       */
  //      void Clear()
  //      {
  //          if (!m_IsUsingSSO && m_HeapData != nullptr)
  //          {
  //              Deallocate(m_HeapData);
  //              m_HeapData = nullptr;
  //          }

  //          m_Size = 0;
  //          m_Capacity = SSO_SIZE;
  //          m_IsUsingSSO = true;
  //          m_SSOBuffer[0] = '\0';
  //      }

  //  private:
  //      void CopyFrom(const String& other)
  //      {
  //          m_Size = other.m_Size;
  //          m_Capacity = other.m_Capacity;
  //          m_Allocator = other.m_Allocator;

  //          if (!other.m_IsUsingSSO)
  //          {
  //              m_HeapData = Allocate(m_Size + 1);
  //              std::memcpy(m_HeapData, other.m_HeapData, m_Size + 1);
  //              m_IsUsingSSO = false;
  //          }
  //          else
  //          {
  //              std::memcpy(m_SSOBuffer, other.m_SSOBuffer, m_Size + 1);
  //              m_IsUsingSSO = true;
  //          }
  //      }

  //  private:
		//char* Allocate(uint64_t size)
		//{
		//	if (m_Allocator == nullptr)
		//	{
		//		char* data = (char*)operator new(size);
		//		memset(data, 0, size);				
		//		return data;
		//	}

		//	return m_Allocator->Allocate<char>(size);
		//}

		//void Deallocate(char* ptr)
		//{
		//	if (m_Allocator == nullptr)
		//	{
		//		delete ptr;
		//		return;
		//	}

		//	m_Allocator->Deallocate<char>(ptr);
		//}

  //  private:
  //      uint32_t m_Size; // Not in bytes
  //      uint32_t m_Capacity; // Not in bytes
  //      union
  //      {
  //          char m_SSOBuffer[SSO_SIZE + 1];
  //          char* m_HeapData;
  //      };
  //      bool m_IsUsingSSO = true;

  //      TAllocator* m_Allocator = nullptr; // Does not own the pointer
  //  };

  //  template<typename TAllocator>
  //  auto MakeString(TAllocator* allocator)
  //  {
  //      return String<TAllocator>(allocator);
  //  }

  //  template<typename TAllocator>
  //  auto MakeString(TAllocator* allocator, const char* str)
  //  {
  //      return String<TAllocator>(allocator, str);
  //  }
}