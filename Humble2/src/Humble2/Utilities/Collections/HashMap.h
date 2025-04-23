#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
    /// <summary>
    /// A key-value data structure that offers O(1) average-time complexity for insertions, deletions, and lookups.
    /// Uses hashing for fast access and collision resolution techniques like linear probing.
    /// </summary>
    /// <typeparam name="TKey">The type of the key.</typeparam>
    /// <typeparam name="TAllocator">The type of the allocator to use.</typeparam>
    /// <typeparam name="TValue">The type of the key.</typeparam>
    template<typename TKey, typename TValue, typename TAllocator = StandardAllocator>
    class HashMap
    {
    private:
        struct Entry
        {
			std::aligned_storage_t<sizeof(TKey), alignof(TKey)> KeyBuffer;
			std::aligned_storage_t<sizeof(TValue), alignof(TValue)> ValueBuffer;

            bool IsOccupied = false;
            bool IsDeleted = false;

			void Construct(const TKey& key, const TValue& value)
			{
				new (&Key()) TKey(key);
				new (&Value()) TValue(value);

				IsOccupied = true;
				IsDeleted = false;
			}

			void Destruct()
			{
				Key().~TKey();
				Value().~TValue();
			}

			TKey& Key() { return reinterpret_cast<TKey&>(KeyBuffer); }
            const TKey& Key() const { return reinterpret_cast<const TKey&>(KeyBuffer); }

            TValue& Value() { return reinterpret_cast<TValue&>(ValueBuffer); }
            const TValue& Value() const { return reinterpret_cast<const TValue&>(ValueBuffer); }
        };

    public:
        /// <summary>
        /// Constructs a HashMap with an optional initial capacity.
        /// </summary>
        /// <param name="initialCapacity">The starting capacity of the map.</param>
        HashMap(uint32_t initialCapacity = 16)
			: m_Capacity(NextPowerOfTwo(initialCapacity)), m_CurrentSize(0), m_Allocator(nullptr)
        {
            m_Data = Allocate(sizeof(Entry) * m_Capacity);
        }

        /// <summary>
        /// Constructs a HashMap with an optional initial capacity and a custom allocator.
        /// </summary>
        /// <param name="allocator">The allocator to use for memory allocation.</param>
        /// <param name="initialCapacity">The starting capacity of the map.</param>
        HashMap(TAllocator* allocator, uint32_t initialCapacity = 16)
			: m_Capacity(NextPowerOfTwo(initialCapacity)), m_CurrentSize(0), m_Allocator(allocator)
        {
            m_Data = Allocate(sizeof(Entry) * m_Capacity);
        }

		/// <summary>
        /// Copy constructor which performs deep copy of the hash map.
        /// </summary>
        HashMap(const HashMap& other)
			: m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_Allocator(other.m_Allocator)
        {
            m_Data = Allocate(sizeof(Entry) * m_Capacity);
            for (uint32_t i = 0; i < m_Capacity; ++i)
            {
                if (other.m_Data[i].IsOccupied && !other.m_Data[i].IsDeleted)
                {
                    m_Data[i].Construct(other.m_Data[i].Key(), other.m_Data[i].Value());
                }
            }
        }

        /// <summary>
		/// Move constructor which transfers ownership of internal data.
		/// </summary>
        HashMap(HashMap&& other) noexcept
            : m_Data(other.m_Data), m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_Allocator(other.m_Allocator)
        {
            other.m_Data = nullptr;
            other.m_CurrentSize = 0;
            other.m_Capacity = 0;
        }

        // Copy Assignment
        HashMap& operator=(const HashMap& other)
        {
            if (this != &other)
            {
                Clear();
                Deallocate(m_Data);

                m_Capacity = other.m_Capacity;
                m_CurrentSize = other.m_CurrentSize;
                m_Allocator = other.m_Allocator;
                m_Data = Allocate(sizeof(Entry) * m_Capacity);

                for (uint32_t i = 0; i < m_Capacity; ++i)
                {
                    if (other.m_Data[i].IsOccupied && !other.m_Data[i].IsDeleted)
                    {
                        m_Data[i].Construct(other.m_Data[i].Key(), other.m_Data[i].Value());
                    }
                }
            }
            return *this;
        }

        // Move Assignment
        HashMap& operator=(HashMap&& other) noexcept
        {
            if (this != &other)
            {
                Clear();
                Deallocate(m_Data);

                m_Data = other.m_Data;
                m_Capacity = other.m_Capacity;
                m_CurrentSize = other.m_CurrentSize;
                m_Allocator = other.m_Allocator;

                other.m_Data = nullptr;
                other.m_Capacity = 0;
                other.m_CurrentSize = 0;
            }
            return *this;
        } 

        /// <summary>
        /// Destructor to release allocated memory.
        /// </summary>
        ~HashMap()
        {
			Clear();
            Deallocate(m_Data);
        }
                
        /// <summary>
        /// Insert a key-value pair into the hash map.
        /// </summary>
        /// <param name="key">The key to insert the value to.</param>
        /// <param name="value">The value to insert to the key.</param>
        void Insert(const TKey& key, const TValue& value)
        {
			if ((m_CurrentSize + 1) > m_Capacity * s_LoadFactor || m_DeletedCount > m_Capacity * 0.2f)
            {
                ReAllocate();
            }

            size_t index = ProbeForInsert(key);
            if (!m_Data[index].IsOccupied || m_Data[index].IsDeleted)
            {
                m_Data[index].Construct(key, value);
                ++m_CurrentSize;
            }
            else
            {
                m_Data[index].Value() = value;
            }
        }
                
        /// <summary>
        /// Find a value by key.
        /// </summary>
        /// <param name="key">The key to find.</param>
        /// <param name="outValue">The value to emplace the found value.</param>
        /// <returns>True if the key was found, false if not found.</returns>
        bool Find(const TKey& key, TValue& outValue) const
        {
            size_t index = ProbeForKey(key);

            if (index != s_InvalidIndex)
            {
                outValue = m_Data[index].Value();
                return true;
            }
            return false;
        }

        /// <summary>
        /// Find if a key exists in the map.
        /// </summary>
        /// <param name="key">The key to find.</param>
        /// <returns>True if the key was found, false if not found.</returns>
        bool ContainsKey(const TKey& key) const
        {
            return ProbeForKey(key) != s_InvalidIndex;
        }

        /// <summary>
        /// Erase a key-value pair by key.
        /// </summary>
        /// <param name="key">The key to remove.</param>
        void Erase(const TKey& key)
        {
            size_t index = ProbeForKey(key);
            if (index != s_InvalidIndex)
            {
                m_Data[index].Destruct();
                --m_CurrentSize;
				++m_DeletedCount;
            }
        }

        /// <summary>
        /// Get the current size of the hash map.
        /// </summary>
        /// <returns>The current size of the hash map.</returns>
        size_t Size() const { return m_CurrentSize; }

        /// <summary>
        /// Get the capacity of the hash map.
        /// </summary>
        /// <returns>The capacity of the hash map.</returns>
        size_t Capacity() const { return m_Capacity; }

        /// <summary>
        /// Clears the entire hash map.
        /// </summary>
        void Clear()
        {
			for (uint32_t i = 0; i < m_Capacity; ++i)
            {
				if (m_Data[i].IsOccupied && !m_Data[i].IsDeleted)
				{
					m_Data[i].Destruct();
					m_Data[i].IsDeleted = true;
				}
            }

            m_CurrentSize = 0;
        }

        /// <summary>
        /// Gets or inserts a value in the given key.
        /// </summary>
        /// <param name="key">The key to get or set.</param>
        /// <returns>The value of the key or default.</returns>
        TValue& operator[](const TKey& key)
        {
			if ((m_CurrentSize + 1) > m_Capacity * s_LoadFactor || m_DeletedCount > m_Capacity * 0.2f)
            {
                ReAllocate();
            }

            size_t index = ProbeForInsert(key);
            if (!m_Data[index].IsOccupied || m_Data[index].IsDeleted)
            {
                m_Data[index].Construct(key, TValue());
                ++m_CurrentSize;
            }

            return m_Data[index].Value();
        }

		class Iterator
		{
		public:
			Iterator(Entry* data, size_t capacity, size_t index)
            : m_Data(data), m_Capacity(capacity), m_Index(index)
			{
				SkipToNextValid();
			}

			std::pair<TKey&, TValue&> operator*() const
			{
				return { m_Data[m_Index].Key(), m_Data[m_Index].Value() };
			}

			Iterator& operator++()
			{
				++m_Index;
				SkipToNextValid();
				return *this;
			}

			bool operator!=(const Iterator& other) const
			{
				return m_Index != other.m_Index || m_Data != other.m_Data;
			}

		private:
			void SkipToNextValid()
			{
				while (m_Index < m_Capacity &&
					(!m_Data[m_Index].IsOccupied || m_Data[m_Index].IsDeleted))
				{
					++m_Index;
				}
			}

			Entry* m_Data;
			size_t m_Capacity;
			size_t m_Index;
		};

		class ConstIterator
		{
		public:
			ConstIterator(const Entry* data, size_t capacity, size_t index)
            : m_Data(data), m_Capacity(capacity), m_Index(index)
			{
				SkipToNextValid();
			}

			std::pair<const TKey&, const TValue&> operator*() const
			{
				return { m_Data[m_Index].Key(), m_Data[m_Index].Value() };
			}

			ConstIterator& operator++()
			{
				++m_Index;
				SkipToNextValid();
				return *this;
			}

			bool operator!=(const ConstIterator& other) const
			{
				return m_Index != other.m_Index || m_Data != other.m_Data;
			}

		private:
			void SkipToNextValid()
			{
				while (m_Index < m_Capacity && (!m_Data[m_Index].IsOccupied || m_Data[m_Index].IsDeleted))
				{
					++m_Index;
				}
			}

			const Entry* m_Data;
			size_t m_Capacity;
			size_t m_Index;
		};

		Iterator begin() { return Iterator(m_Data, m_Capacity, 0); }
		Iterator end() { return Iterator(m_Data, m_Capacity, m_Capacity); }

		ConstIterator begin() const { return ConstIterator(m_Data, m_Capacity, 0); }
		ConstIterator end() const { return ConstIterator(m_Data, m_Capacity, m_Capacity); }

		ConstIterator cbegin() const { return ConstIterator(m_Data, m_Capacity, 0); }
		ConstIterator cend() const { return ConstIterator(m_Data, m_Capacity, m_Capacity); }
        
    private:
		static constexpr float s_LoadFactor = 0.7f;
        static constexpr size_t s_InvalidIndex = static_cast<size_t>(-1);

		size_t NextPowerOfTwo(size_t n)
        {
            size_t p = 1;
            while (p < n) p <<= 1;
            return p;
        }

        size_t ProbeForInsert(const TKey& key) const
        {
            size_t index = Hash(key) & (m_Capacity - 1);
            while (m_Data[index].IsOccupied && !m_Data[index].IsDeleted && !(m_Data[index].Key() == key))
            {
                index = (index + 1) & (m_Capacity - 1);
            }
            return index;
        }

        size_t ProbeForKey(const TKey& key) const
        {
            size_t index = Hash(key) & (m_Capacity - 1);
            size_t start = index;
            do
            {
				if (!m_Data[index].IsOccupied)
				{
					return s_InvalidIndex;
				}

				if (m_Data[index].Key() == key && !m_Data[index].IsDeleted)
				{
					return index;
				}

                index = (index + 1) & (m_Capacity - 1);
            } while (index != start);

            return s_InvalidIndex;
        }

        size_t Hash(const TKey& key) const
        {
            return std::hash<TKey>()(key);
        }

        void ReAllocate()
        {
            size_t newCapacity = m_Capacity * 2;
            Entry* newData = Allocate(sizeof(Entry) * newCapacity);
            HBL2_CORE_ASSERT(newData, "HashMap reallocation failed!");

            for (size_t i = 0; i < m_Capacity; ++i)
            {
                if (m_Data[i].IsOccupied && !m_Data[i].IsDeleted)
                {
                    size_t index = Hash(m_Data[i].Key()) & (newCapacity - 1);
                    while (newData[index].IsOccupied)
                    {
                        index = (index + 1) & (newCapacity - 1);
                    }
                    newData[index].Construct(m_Data[i].Key(), m_Data[i].Value());
                }
            }

            for (size_t i = 0; i < m_Capacity; ++i)
            {
                if (m_Data[i].IsOccupied && !m_Data[i].IsDeleted)
                {
                    m_Data[i].Destruct();
                }
            }

            Deallocate(m_Data);
            m_Data = newData;
            m_Capacity = newCapacity;
			m_DeletedCount = 0;
        }

    private:
		Entry* Allocate(uint64_t size)
		{
			if (m_Allocator == nullptr)
			{
				Entry* data = (Entry*)std::malloc(size);
				memset(data, 0, size);				
				return data;
			}

			return m_Allocator->Allocate<Entry>(size);
		}

		void Deallocate(Entry* ptr)
		{
			if (m_Allocator == nullptr)
			{
                if (ptr != nullptr)
                {
				    std::free(ptr);
                }

				return;
			}

			m_Allocator->Deallocate<Entry>(ptr);
		}

    private:
        Entry* m_Data;
        uint32_t m_Capacity; // Not in bytes
        uint32_t m_CurrentSize; // Not in bytes
        uint32_t m_DeletedCount = 0;
        TAllocator* m_Allocator = nullptr; // Does not own the pointer
    };

    template<typename TKey, typename TValue, typename TAllocator>
    auto MakeHashMap(TAllocator* allocator, uint32_t initialCapacity = 8)
    {
        return HashMap<TKey, TValue, TAllocator>(allocator, initialCapacity);
    }
}