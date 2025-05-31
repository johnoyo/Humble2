#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>
#include <type_traits>
#include <utility>
#include <functional>

namespace HBL2
{
    /**
     * @brief A key-value data structure that offers O(1) average-time complexity for insertions, deletions, and lookups.
     *
     * Uses hashing for fast access and collision resolution techniques like linear probing.
     *
     * @tparam TKey The type of the key.
     * @tparam TValue The type of the value.
     * @tparam TAllocator The allocator type to use.
     */
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

            void Construct(TKey&& key, TValue&& value)
            {
                new (&Key()) TKey(std::move(key));
                new (&Value()) TValue(std::move(value));

                IsOccupied = true;
                IsDeleted = false;
            }

            template<typename... KeyArgs, typename... ValueArgs>
            void Emplace(std::tuple<KeyArgs...>&& keyArgs, std::tuple<ValueArgs...>&& valueArgs)
            {
                ConstructFromTuple(std::move(keyArgs), std::move(valueArgs), std::index_sequence_for<KeyArgs...>{}, std::index_sequence_for<ValueArgs...>{});

                IsOccupied = true;
                IsDeleted = false;
            }

            void Destruct()
            {
                if constexpr (!std::is_trivially_destructible_v<TKey>)
                {
                    Key().~TKey();
                }
                if constexpr (!std::is_trivially_destructible_v<TValue>)
                {
                    Value().~TValue();
                }

                IsOccupied = false;
                IsDeleted = false;
            }

            void MarkDeleted()
            {
                if constexpr (!std::is_trivially_destructible_v<TKey>)
                {
                    Key().~TKey();
                }
                if constexpr (!std::is_trivially_destructible_v<TValue>)
                {
                    Value().~TValue();
                }

                IsOccupied = false;
                IsDeleted = true;
            }

            TKey& Key() { return reinterpret_cast<TKey&>(KeyBuffer); }
            const TKey& Key() const { return reinterpret_cast<const TKey&>(KeyBuffer); }

            TValue& Value() { return reinterpret_cast<TValue&>(ValueBuffer); }
            const TValue& Value() const { return reinterpret_cast<const TValue&>(ValueBuffer); }

        private:
            template<typename... KeyArgs, typename... ValueArgs, size_t... KeyIs, size_t... ValueIs>
            void ConstructFromTuple(std::tuple<KeyArgs...>&& keyArgs, std::tuple<ValueArgs...>&& valueArgs, std::index_sequence<KeyIs...>, std::index_sequence<ValueIs...>)
            {
                new (&Key()) TKey(std::forward<KeyArgs>(std::get<KeyIs>(keyArgs))...);
                new (&Value()) TValue(std::forward<ValueArgs>(std::get<ValueIs>(valueArgs))...);
            }
        };

    public:
        /**
         * @brief Constructs a HashMap with an optional initial capacity.
         *
         * @param initialCapacity The starting capacity of the map (default 16).
         */
        HashMap(uint32_t initialCapacity = 16)
            : m_Capacity(NextPowerOfTwo(initialCapacity)), m_CurrentSize(0), m_DeletedCount(0), m_Allocator(nullptr)
        {
            m_Data = Allocate(sizeof(Entry) * m_Capacity);
            InitializeEntries(m_Data, m_Capacity);
        }

        /**
         * @brief Constructs a HashMap with an optional initial capacity and a custom allocator.
         *
         * @param allocator The allocator to use for memory allocation.
         * @param initialCapacity The starting capacity of the map (default 16).
         */
        HashMap(TAllocator* allocator, uint32_t initialCapacity = 16)
            : m_Capacity(NextPowerOfTwo(initialCapacity)), m_CurrentSize(0), m_DeletedCount(0), m_Allocator(allocator)
        {
            m_Data = Allocate(sizeof(Entry) * m_Capacity);
            InitializeEntries(m_Data, m_Capacity);
        }

        /**
         * @brief Copy constructor which performs deep copy of the hash map.
         *
         * @param other The HashMap to copy from.
         */
        HashMap(const HashMap& other)
            : m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_DeletedCount(other.m_DeletedCount), m_Allocator(other.m_Allocator)
        {
            m_Data = Allocate(sizeof(Entry) * m_Capacity);
            InitializeEntries(m_Data, m_Capacity);

            for (uint32_t i = 0; i < m_Capacity; ++i)
            {
                if (other.m_Data[i].IsOccupied && !other.m_Data[i].IsDeleted)
                {
                    m_Data[i].Construct(other.m_Data[i].Key(), other.m_Data[i].Value());
                }
                else
                {
                    m_Data[i].IsDeleted = other.m_Data[i].IsDeleted;
                }
            }
        }

        /**
         * @brief Move constructor which transfers ownership of internal data.
         *
         * @param other The HashMap to move from.
         */
        HashMap(HashMap&& other) noexcept
            : m_Data(other.m_Data), m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_DeletedCount(other.m_DeletedCount), m_Allocator(other.m_Allocator)
        {
            other.m_Data = nullptr;
            other.m_CurrentSize = 0;
            other.m_Capacity = 0;
            other.m_DeletedCount = 0;
            other.m_Allocator = nullptr;
        }

        /**
         * @brief Copy assignment operator.
         *
         * @param other The HashMap to copy from.
         * @return Reference to this HashMap.
         */
        HashMap& operator=(const HashMap& other)
        {
            if (this != &other)
            {
                Clear();
                Deallocate(m_Data);

                m_Capacity = other.m_Capacity;
                m_CurrentSize = other.m_CurrentSize;
                m_DeletedCount = other.m_DeletedCount;
                m_Allocator = other.m_Allocator;

                m_Data = Allocate(sizeof(Entry) * m_Capacity);
                InitializeEntries(m_Data, m_Capacity);

                for (uint32_t i = 0; i < m_Capacity; ++i)
                {
                    if (other.m_Data[i].IsOccupied && !other.m_Data[i].IsDeleted)
                    {
                        m_Data[i].Construct(other.m_Data[i].Key(), other.m_Data[i].Value());
                    }
                    else
                    {
                        m_Data[i].IsDeleted = other.m_Data[i].IsDeleted;
                    }
                }
            }
            return *this;
        }

        /**
         * @brief Move assignment operator.
         *
         * @param other The HashMap to move from.
         * @return Reference to this HashMap.
         */
        HashMap& operator=(HashMap&& other) noexcept
        {
            if (this != &other)
            {
                Clear();
                Deallocate(m_Data);

                m_Data = other.m_Data;
                m_Capacity = other.m_Capacity;
                m_CurrentSize = other.m_CurrentSize;
                m_DeletedCount = other.m_DeletedCount;
                m_Allocator = other.m_Allocator;

                other.m_Data = nullptr;
                other.m_Capacity = 0;
                other.m_CurrentSize = 0;
                other.m_DeletedCount = 0;
                other.m_Allocator = nullptr;
            }
            return *this;
        }

        /**
         * @brief Destructor to release allocated memory.
         */
        ~HashMap()
        {
            Clear();
            Deallocate(m_Data);
        }

        /**
         * @brief Insert a key-value pair into the hash map.
         *
         * @param key The key to insert the value to.
         * @param value The value to insert for the key.
         */
        void Insert(const TKey& key, const TValue& value)
        {
            if ((m_CurrentSize + 1) > m_Capacity * s_LoadFactor || m_DeletedCount > m_Capacity * 0.2f)
            {
                ReAllocate();
            }

            size_t index = ProbeForInsert(key);
            if (!m_Data[index].IsOccupied || m_Data[index].IsDeleted)
            {
                if (m_Data[index].IsDeleted)
                {
                    --m_DeletedCount;
                }
                m_Data[index].Construct(key, value);
                ++m_CurrentSize;
            }
            else
            {
                // Key already exists, update value
                m_Data[index].Value() = value;
            }
        }

        /**
         * @brief Insert a key-value pair into the hash map (move version).
         *
         * @param key The key to move into the map.
         * @param value The value to move into the map.
         */
        void Insert(TKey&& key, TValue&& value)
        {
            if ((m_CurrentSize + 1) > m_Capacity * s_LoadFactor || m_DeletedCount > m_Capacity * 0.2f)
            {
                ReAllocate();
            }

            size_t index = ProbeForInsert(key);
            if (!m_Data[index].IsOccupied || m_Data[index].IsDeleted)
            {
                if (m_Data[index].IsDeleted)
                {
                    --m_DeletedCount;
                }
                m_Data[index].Construct(std::move(key), std::move(value));
                ++m_CurrentSize;
            }
            else
            {
                // Key already exists, update value
                m_Data[index].Value() = std::move(value);
            }
        }

        /**
         * @brief Emplace a key-value pair into the hash map.
         *
         * @tparam KeyArgs Types of key constructor arguments.
         * @tparam ValueArgs Types of value constructor arguments.
         * @param keyArgs Arguments for key construction.
         * @param valueArgs Arguments for value construction.
         */
        template<typename... KeyArgs, typename... ValueArgs>
        void Emplace(std::tuple<KeyArgs...> keyArgs, std::tuple<ValueArgs...> valueArgs)
        {
            if ((m_CurrentSize + 1) > m_Capacity * s_LoadFactor || m_DeletedCount > m_Capacity * 0.2f)
            {
                ReAllocate();
            }

            // We need to construct a temporary key to find the insertion point
            // This is a limitation of the current design
            TKey tempKey(std::forward<KeyArgs>(std::get<KeyArgs>(keyArgs))...);

            size_t index = ProbeForInsert(tempKey);
            if (!m_Data[index].IsOccupied || m_Data[index].IsDeleted)
            {
                if (m_Data[index].IsDeleted)
                {
                    --m_DeletedCount;
                }
                m_Data[index].Emplace(std::move(keyArgs), std::move(valueArgs));
                ++m_CurrentSize;
            }
        }

        /**
         * @brief Find a value by key.
         *
         * @param key The key to find.
         * @param outValue Reference to store the found value.
         * @return True if the key was found, false otherwise.
         */
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

        /**
         * @brief Find a value by key and return a pointer to it.
         *
         * @param key The key to find.
         * @return Pointer to the value if found, nullptr otherwise.
         */
        TValue* Find(const TKey& key)
        {
            size_t index = ProbeForKey(key);
            return (index != s_InvalidIndex) ? &m_Data[index].Value() : nullptr;
        }

        /**
         * @brief Find a value by key and return a const pointer to it.
         *
         * @param key The key to find.
         * @return Const pointer to the value if found, nullptr otherwise.
         */
        const TValue* Find(const TKey& key) const
        {
            size_t index = ProbeForKey(key);
            return (index != s_InvalidIndex) ? &m_Data[index].Value() : nullptr;
        }

        /**
         * @brief Check if a key exists in the map.
         *
         * @param key The key to find.
         * @return True if the key exists, false otherwise.
         */
        bool ContainsKey(const TKey& key) const
        {
            return ProbeForKey(key) != s_InvalidIndex;
        }

        /**
         * @brief Erase a key-value pair by key.
         *
         * @param key The key to remove.
         * @return True if the key was found and removed, false otherwise.
         */
        bool Erase(const TKey& key)
        {
            size_t index = ProbeForKey(key);
            if (index != s_InvalidIndex)
            {
                m_Data[index].MarkDeleted();
                --m_CurrentSize;
                ++m_DeletedCount;
                return true;
            }
            return false;
        }

        /**
         * @brief Get the current size of the hash map.
         *
         * @return The number of key-value pairs stored.
         */
        uint32_t Size() const { return m_CurrentSize; }

        /**
         * @brief Get the capacity of the hash map.
         *
         * @return The total capacity of the hash map.
         */
        uint32_t Capacity() const { return m_Capacity; }

        /**
         * @brief Check if the hash map is empty.
         *
         * @return True if empty, false otherwise.
         */
        bool Empty() const { return m_CurrentSize == 0; }

        /**
         * @brief Get the current load factor of the hash map.
         *
         * @return The load factor (size / capacity).
         */
        float LoadFactor() const { return static_cast<float>(m_CurrentSize) / static_cast<float>(m_Capacity); }

        /**
         * @brief Clears the entire hash map, destructing all entries.
         */
        void Clear()
        {
            if constexpr (std::is_trivially_destructible_v<TKey> && std::is_trivially_destructible_v<TValue>)
            {
                // Fast path for POD types - just reset flags
                for (uint32_t i = 0; i < m_Capacity; ++i)
                {
                    m_Data[i].IsOccupied = false;
                    m_Data[i].IsDeleted = false;
                }
            }
            else
            {
                // Call destructors for non-POD types
                for (uint32_t i = 0; i < m_Capacity; ++i)
                {
                    if (m_Data[i].IsOccupied && !m_Data[i].IsDeleted)
                    {
                        m_Data[i].Destruct();
                    }
                    else
                    {
                        m_Data[i].IsOccupied = false;
                        m_Data[i].IsDeleted = false;
                    }
                }
            }

            m_CurrentSize = 0;
            m_DeletedCount = 0;
        }

        /**
         * @brief Reserve capacity for at least the specified number of elements.
         *
         * @param newCapacity The minimum capacity to reserve.
         */
        void Reserve(uint32_t newCapacity)
        {
            if (newCapacity > m_Capacity)
            {
                uint32_t actualCapacity = NextPowerOfTwo(newCapacity);
                ReAllocate(actualCapacity);
            }
        }

        /**
         * @brief Gets or inserts a value for the given key.
         *
         * @param key The key to get or set.
         * @return Reference to the value associated with the key.
         */
        TValue& operator[](const TKey& key)
        {
            if ((m_CurrentSize + 1) > m_Capacity * s_LoadFactor || m_DeletedCount > m_Capacity * 0.2f)
            {
                ReAllocate();
            }

            size_t index = ProbeForInsert(key);
            if (!m_Data[index].IsOccupied || m_Data[index].IsDeleted)
            {
                if (m_Data[index].IsDeleted)
                {
                    --m_DeletedCount;
                }
                m_Data[index].Construct(key, TValue{});
                ++m_CurrentSize;
            }

            return m_Data[index].Value();
        }

        class Iterator
        {
        public:
            Iterator(Entry* data, uint32_t capacity, uint32_t index)
                : m_Data(data), m_Capacity(capacity), m_Index(index)
            {
                SkipToNextValid();
            }

            std::pair<TKey&, TValue&> operator*() const
            {
                return { m_Data[m_Index].Key(), m_Data[m_Index].Value() };
            }

            std::pair<TKey*, TValue*> operator->() const
            {
                return { &m_Data[m_Index].Key(), &m_Data[m_Index].Value() };
            }

            Iterator& operator++()
            {
                ++m_Index;
                SkipToNextValid();
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator temp = *this;
                ++(*this);
                return temp;
            }

            bool operator==(const Iterator& other) const
            {
                return m_Index == other.m_Index && m_Data == other.m_Data;
            }

            bool operator!=(const Iterator& other) const
            {
                return !(*this == other);
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
            uint32_t m_Capacity;
            uint32_t m_Index;
        };

        class ConstIterator
        {
        public:
            ConstIterator(const Entry* data, uint32_t capacity, uint32_t index)
                : m_Data(data), m_Capacity(capacity), m_Index(index)
            {
                SkipToNextValid();
            }

            std::pair<const TKey&, const TValue&> operator*() const
            {
                return { m_Data[m_Index].Key(), m_Data[m_Index].Value() };
            }

            std::pair<const TKey*, const TValue*> operator->() const
            {
                return { &m_Data[m_Index].Key(), &m_Data[m_Index].Value() };
            }

            ConstIterator& operator++()
            {
                ++m_Index;
                SkipToNextValid();
                return *this;
            }

            ConstIterator operator++(int)
            {
                ConstIterator temp = *this;
                ++(*this);
                return temp;
            }

            bool operator==(const ConstIterator& other) const
            {
                return m_Index == other.m_Index && m_Data == other.m_Data;
            }

            bool operator!=(const ConstIterator& other) const
            {
                return !(*this == other);
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

            const Entry* m_Data;
            uint32_t m_Capacity;
            uint32_t m_Index;
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

        uint32_t NextPowerOfTwo(uint32_t n) const
        {
            if (n <= 1) return 1;

            uint32_t p = 1;
            while (p < n) p <<= 1;
            return p;
        }

        void InitializeEntries(Entry* data, uint32_t capacity)
        {
            if constexpr (std::is_trivially_constructible_v<Entry>)
            {
                // Fast path for trivial Entry initialization
                std::memset(data, 0, capacity * sizeof(Entry));
            }
            else
            {
                // Proper initialization for non-trivial Entry
                for (uint32_t i = 0; i < capacity; ++i)
                {
                    new (&data[i]) Entry{};
                }
            }
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

                if (!m_Data[index].IsDeleted && m_Data[index].Key() == key)
                {
                    return index;
                }

                index = (index + 1) & (m_Capacity - 1);
            } while (index != start);

            return s_InvalidIndex;
        }

        size_t Hash(const TKey& key) const
        {
            return std::hash<TKey>{}(key);
        }

        void ReAllocate(uint32_t newCapacity = 0)
        {
            if (newCapacity == 0)
            {
                newCapacity = m_Capacity * 2;
            }

            Entry* newData = Allocate(sizeof(Entry) * newCapacity);
            HBL2_CORE_ASSERT(newData, "HashMap reallocation failed!");
            InitializeEntries(newData, newCapacity);

            // Rehash all existing entries
            for (uint32_t i = 0; i < m_Capacity; ++i)
            {
                if (m_Data[i].IsOccupied && !m_Data[i].IsDeleted)
                {
                    size_t index = Hash(m_Data[i].Key()) & (newCapacity - 1);
                    while (newData[index].IsOccupied)
                    {
                        index = (index + 1) & (newCapacity - 1);
                    }

                    if constexpr (std::is_trivially_copyable_v<TKey> && std::is_trivially_copyable_v<TValue>)
                    {
                        newData[index].Construct(m_Data[i].Key(), m_Data[i].Value());
                    }
                    else
                    {
                        newData[index].Construct(std::move(m_Data[i].Key()), std::move(m_Data[i].Value()));
                    }
                }
            }

            // Clean up old data
            if constexpr (!std::is_trivially_destructible_v<TKey> || !std::is_trivially_destructible_v<TValue>)
            {
                for (uint32_t i = 0; i < m_Capacity; ++i)
                {
                    if (m_Data[i].IsOccupied && !m_Data[i].IsDeleted)
                    {
                        m_Data[i].Destruct();
                    }
                }
            }

            Deallocate(m_Data);
            m_Data = newData;
            m_Capacity = newCapacity;
            m_DeletedCount = 0;
        }

        Entry* Allocate(uint64_t size)
        {
            if (m_Allocator == nullptr)
            {
                return static_cast<Entry*>(operator new(size));
            }

            return m_Allocator->Allocate<Entry>(size);
        }

        void Deallocate(Entry* ptr)
        {
            if (ptr == nullptr) return;

            if (m_Allocator == nullptr)
            {
                operator delete(ptr);
                return;
            }

            m_Allocator->Deallocate<Entry>(ptr);
        }

    private:
        Entry* m_Data = nullptr;
        uint32_t m_Capacity = 0;
        uint32_t m_CurrentSize = 0;
        uint32_t m_DeletedCount = 0;
        TAllocator* m_Allocator = nullptr;
    };

    /**
     * @brief Helper function to create a HashMap with a custom allocator.
     *
     * @tparam TKey The key type.
     * @tparam TValue The value type.
     * @tparam TAllocator The allocator type.
     * @param allocator Pointer to the allocator.
     * @param initialCapacity Initial capacity of the map.
     * @return A new HashMap instance.
     */
    template<typename TKey, typename TValue, typename TAllocator>
    auto MakeHashMap(TAllocator* allocator, uint32_t initialCapacity = 16)
    {
        return HashMap<TKey, TValue, TAllocator>(allocator, initialCapacity);
    }
}