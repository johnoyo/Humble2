#pragma once

#include "Base.h"
#include "Utilities/Allocators/Arena.h"

#include <cstdint>
#include <cstring>
#include <utility>
#include <type_traits>
#include <functional>

namespace HBL2
{
    template<typename K>
    struct Hash { size_t operator()(const K& inKey) const { return std::hash<K>{}(inKey); } };

    /// Fixed-capacity hash map backed by an arena.
    /// Uses open addressing with linear probing and a separate control-byte array (Swiss Table layout).
    /// Capacity must be a power of 2. Recommended: capacity >= expected_elements * 2.
    template<typename K, typename V, typename Hasher = Hash<K>>
    class HashMap
    {
    public:
        using key_type = K;
        using value_type = std::pair<const K, V>;
        using size_type = uint32_t;

    private:
        // Control byte values (bottom 7 bits store part of the hash — filters false positives before key compare)
        static constexpr uint8_t cEmpty = 0x00;
        static constexpr uint8_t cDeleted = 0x7F;
        static constexpr uint8_t cUsed = 0x80; // bit 7 set = occupied; bits 0-6 = low 7 bits of hash

        struct alignas(value_type) Storage
        {
            uint8_t Data[sizeof(value_type)];
        };

        static_assert(sizeof(value_type) == sizeof(Storage));
        static_assert(alignof(value_type) == alignof(Storage));

        value_type& GetElement(size_type inIdx) { return reinterpret_cast<value_type&>(m_Data[inIdx]); }
        const value_type& GetElement(size_type inIdx) const { return reinterpret_cast<const value_type&>(m_Data[inIdx]); }

    public:

        // Stores a pointer to the slot directly so operator* and operator-> can return real references.
        template<bool IsConst>
        class IteratorBase
        {
            using TablePtr = std::conditional_t<IsConst, const HashMap*, HashMap*>;
            using KVRef = std::conditional_t<IsConst, const value_type&, value_type&>;
            using KVPtr = std::conditional_t<IsConst, const value_type*, value_type*>;

        public:
            IteratorBase() = default;
            IteratorBase(TablePtr inTable, size_type inIdx) : m_Table(inTable), m_Idx(inIdx) {}

            // Implicit conversion from non-const to const iterator
            template<bool B, typename = std::enable_if_t<IsConst && !B>>
            IteratorBase(const IteratorBase<B>& inOther)
                : m_Table(inOther.m_Table), m_Idx(inOther.m_Idx) {
            }

            IteratorBase& operator++()
            {
                do { ++m_Idx; } while (m_Idx < m_Table->m_Capacity && (m_Table->m_Control[m_Idx] & cUsed) == 0);
                return *this;
            }

            IteratorBase operator++(int)
            {
                IteratorBase tmp = *this;
                ++(*this);
                return tmp;
            }

            KVRef operator*()  const { HBL2_CORE_ASSERT(IsValid(), ""); return m_Table->GetElement(m_Idx); }
            KVPtr operator->() const { HBL2_CORE_ASSERT(IsValid(), ""); return &m_Table->GetElement(m_Idx); }

            bool operator==(const IteratorBase& inRHS) const { return m_Idx == inRHS.m_Idx && m_Table == inRHS.m_Table; }
            bool operator!=(const IteratorBase& inRHS) const { return !(*this == inRHS); }
            bool IsValid() const { return m_Table && m_Idx < m_Table->m_Capacity && (m_Table->m_Control[m_Idx] & cUsed); }

            template<bool> friend class IteratorBase;
            friend class HashMap;

        private:
            TablePtr  m_Table = nullptr;
            size_type m_Idx = 0;
        };

        using iterator = IteratorBase<false>;
        using const_iterator = IteratorBase<true>;

        HashMap() = default;

        explicit HashMap(Arena* arena, size_type inCapacity)
            : m_Arena(arena)
        {
            HBL2_CORE_ASSERT(inCapacity > 0 && (inCapacity & (inCapacity - 1)) == 0, "HashMap capacity must be a power of 2.");
            Allocate(inCapacity);
        }

        HashMap(const HashMap& inRHS)
            : m_Arena(inRHS.m_Arena)
        {
            if (inRHS.empty())
            {
                return;
            }

            Allocate(inRHS.m_Capacity);
            CopyFrom(inRHS);
        }

        HashMap(HashMap&& inRHS) noexcept
            : m_Arena(inRHS.m_Arena), m_Capacity(inRHS.m_Capacity), m_Size(inRHS.m_Size), m_Data(inRHS.m_Data), m_Control(inRHS.m_Control)
        {
            inRHS.m_Arena = nullptr;
            inRHS.m_Capacity = 0;
            inRHS.m_Size = 0;
            inRHS.m_Data = nullptr;
            inRHS.m_Control = nullptr;
        }

        ~HashMap() { clear(); }

        bool empty()    const { return m_Size == 0; }
        size_type size()     const { return m_Size; }
        size_type capacity() const { return m_Capacity; }

        void clear()
        {
            if (!m_Data)
            {
                return;
            }

            if constexpr (!std::is_trivially_destructible_v<value_type>)
            {
                for (size_type i = 0; i < m_Capacity; ++i)
                {
                    if (m_Control[i] & cUsed)
                    {
                        GetElement(i).~value_type();
                    }
                }
            }

            std::memset(m_Control, cEmpty, m_Capacity);
            m_Size = 0;
        }

        /// Insert or overwrite. Returns {iterator, true} if inserted, {iterator, false} if already existed.
        std::pair<iterator, bool> insert(const K& inKey, const V& inValue)
        {
            auto [idx, inserted] = FindOrAllocate(inKey);
            if (inserted)
            {
                new (&m_Data[idx]) value_type(inKey, inValue);
            }
            else
            {
                GetElement(idx).second = inValue;
            }

            return { iterator(this, idx), inserted };
        }

        /// Construct value in-place. Returns {iterator, true} if inserted, {iterator, false} if already existed.
        template<typename... Args>
        std::pair<iterator, bool> emplace(const K& inKey, Args&&... inArgs)
        {
            auto [idx, inserted] = FindOrAllocate(inKey);
            if (inserted)
            {
                new (&m_Data[idx]) value_type(std::piecewise_construct, std::forward_as_tuple(inKey), std::forward_as_tuple(std::forward<Args>(inArgs)...));
            }
            else
            {
                GetElement(idx).second.~V();
                new (&GetElement(idx).second) V(std::forward<Args>(inArgs)...);
            }
            return { iterator(this, idx), inserted };
        }

        /// Remove by key. Returns true if the key was found and removed.
        bool remove(const K& inKey)
        {
            size_type idx = ProbeFind(inKey);
            if (idx == cInvalid)
            {
                return false;
            }

            EraseAt(idx);

            return true;
        }

        void erase(const_iterator inIter)
        {
            HBL2_CORE_ASSERT(inIter.IsValid(), "");
            EraseAt(inIter.m_Idx);
        }

        iterator find(const K& inKey)
        {
            size_type idx = ProbeFind(inKey);
            return idx != cInvalid ? iterator(this, idx) : end();
        }

        const_iterator find(const K& inKey) const
        {
            size_type idx = ProbeFind(inKey);
            return idx != cInvalid ? const_iterator(this, idx) : end();
        }

        bool contains(const K& inKey) const { return ProbeFind(inKey) != cInvalid; }

        /// Returns value for key, default-constructing it if absent.
        V& operator[](const K& inKey)
        {
            auto [idx, inserted] = FindOrAllocate(inKey);
            if (inserted)
            {
                new (&m_Data[idx]) value_type(inKey, V{});
            }

            return GetElement(idx).second;
        }

        iterator begin()
        {
            size_type i = 0;
            while (i < m_Capacity && (m_Control[i] & cUsed) == 0) ++i;
            return iterator(this, i);
        }

        iterator end() { return iterator(this, m_Capacity); }
        const_iterator begin() const
        {
            size_type i = 0;
            while (i < m_Capacity && (m_Control[i] & cUsed) == 0) ++i;
            return const_iterator(this, i);
        }
        const_iterator end() const { return const_iterator(this, m_Capacity); }
        const_iterator cbegin() const { return begin(); }
        const_iterator cend() const { return end(); }

        HashMap& operator=(const HashMap& inRHS)
        {
            if (this == &inRHS)
            {
                return *this;
            }

            clear();

            m_Arena = inRHS.m_Arena;
            if (!inRHS.empty())
            {
                if (m_Capacity != inRHS.m_Capacity)
                {
                    m_Data = nullptr;
                    m_Control = nullptr;
                    Allocate(inRHS.m_Capacity);
                }

                CopyFrom(inRHS);
            }

            return *this;
        }

        HashMap& operator=(HashMap&& inRHS) noexcept
        {
            if (this == &inRHS)
            {
                return *this;
            }

            clear();

            m_Arena = inRHS.m_Arena;
            m_Capacity = inRHS.m_Capacity;
            m_Size = inRHS.m_Size;
            m_Data = inRHS.m_Data;
            m_Control = inRHS.m_Control;
            inRHS.m_Arena = nullptr;
            inRHS.m_Capacity = 0;
            inRHS.m_Size = 0;
            inRHS.m_Data = nullptr;
            inRHS.m_Control = nullptr;

            return *this;
        }

    private:
        static constexpr size_type cInvalid = ~size_type(0);

        /// Split hash: high bits → bucket index, low 7 bits → control tag
        void HashToIndexAndControl(const K& inKey, size_type& outIdx, uint8_t& outControl) const
        {
            size_t h = Hasher{}(inKey);
            outIdx = size_type((h >> 7) & (m_Capacity - 1)); // power-of-2 bitmask
            outControl = cUsed | uint8_t(h & 0x7F);
        }

        /// Find an occupied slot by key. Returns cInvalid if not found.
        size_type ProbeFind(const K& inKey) const
        {
            if (!m_Data || m_Capacity == 0)
            {
                return cInvalid;
            }

            size_type idx;
            uint8_t ctrl;
            HashToIndexAndControl(inKey, idx, ctrl);

            const size_type mask = m_Capacity - 1;
            for (size_type i = 0; i < m_Capacity; ++i)
            {
                const uint8_t c = m_Control[idx];
                if (c == cEmpty)
                {
                    return cInvalid;   // true gap, key cannot be beyond this
                }

                if (c == ctrl && GetElement(idx).first == inKey)
                {
                    return idx;
                }

                idx = (idx + 1) & mask;
            }

            return cInvalid;
        }

        /// Find existing key or allocate a new slot. Returns {index, wasInserted}.
        std::pair<size_type, bool> FindOrAllocate(const K& inKey)
        {
            HBL2_CORE_ASSERT(m_Data && m_Capacity > 0, "HashMap used before construction.");
            HBL2_CORE_ASSERT(m_Size * 8 < m_Capacity * 7, "HashMap load factor exceeded 87.5% — increase capacity.");

            size_type idx;
            uint8_t ctrl;
            HashToIndexAndControl(inKey, idx, ctrl);

            const size_type mask = m_Capacity - 1;
            size_type firstTombstone = cInvalid;

            for (size_type i = 0; i < m_Capacity; ++i)
            {
                const uint8_t c = m_Control[idx];
                if (c == cEmpty)
                {
                    // Reuse tombstone if we passed one; it sits earlier in the probe chain
                    size_type dst = (firstTombstone != cInvalid) ? firstTombstone : idx;
                    m_Control[dst] = ctrl;
                    ++m_Size;
                    return { dst, true };
                }

                if (c == cDeleted && firstTombstone == cInvalid)
                {
                    firstTombstone = idx;
                }

                if (c == ctrl && GetElement(idx).first == inKey)
                {
                    return { idx, false };  // already exists
                }

                idx = (idx + 1) & mask;
            }

            // Should never reach here if load-factor assert above is respected
            HBL2_CORE_ASSERT(false, "HashMap is full.");
            return { cInvalid, false };
        }

        void EraseAt(size_type inIdx)
        {
            HBL2_CORE_ASSERT(m_Control[inIdx] & cUsed, "");
            GetElement(inIdx).~value_type();

            // Mark as deleted (tombstone) unless the next slot is empty — then we can
            // mark it empty too, shrinking the probe chain.
            const size_type next = (inIdx + 1) & (m_Capacity - 1);
            m_Control[inIdx] = (m_Control[next] == cEmpty) ? cEmpty : cDeleted;
            --m_Size;
        }

        void Allocate(size_type inCapacity)
        {
            HBL2_CORE_ASSERT(m_Arena, "");
            m_Capacity = inCapacity;
            // Pack data + control into one allocation. Data first (aligned), control bytes after.
            m_Data = static_cast<Storage*>(m_Arena->Alloc(sizeof(Storage) * inCapacity + inCapacity));
            m_Control = reinterpret_cast<uint8_t*>(m_Data + inCapacity);
            std::memset(m_Control, cEmpty, inCapacity);
        }

        void CopyFrom(const HashMap& inRHS)
        {
            std::memcpy(m_Control, inRHS.m_Control, m_Capacity);
            for (size_type i = 0; i < m_Capacity; ++i)
            {
                if (m_Control[i] & cUsed)
                {
                    new (&m_Data[i]) value_type(inRHS.GetElement(i));
                }
            }

            m_Size = inRHS.m_Size;
        }

        Arena* m_Arena = nullptr;
        size_type  m_Capacity = 0;
        size_type  m_Size = 0;
        Storage* m_Data = nullptr;
        uint8_t* m_Control = nullptr;
    };
}