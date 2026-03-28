#pragma once

#include "Base.h"
#include "Utilities/Allocators/Arena.h"

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace HBL2
{
    template<typename K>
    struct Hash
    {
        size_t operator()(const K& inKey) const
        {
            return std::hash<K>{}(inKey);
        }
    };

    /// Fixed-capacity hash map backed by an arena.
    /// Uses open addressing with linear probing and tombstone deletion.
    /// Capacity should be ~1.5–2× the expected element count to stay under the 70% load factor limit.
    template<typename K, typename V, typename Hasher = Hash<K>>
    class HashMap
    {
    public:
        using key_type = K;
        using value_type = V;
        using size_type = uint32_t;

    private:
        enum class SlotState : uint8_t { Empty, Occupied, Deleted };

        struct Slot
        {
            struct alignas(K) KStorage { uint8_t Data[sizeof(K)]; };
            struct alignas(V) VStorage { uint8_t Data[sizeof(V)]; };

            KStorage   Key;
            VStorage   Value;
            SlotState  State = SlotState::Empty;

            K& GetKey() { return reinterpret_cast<K&>(Key); }
            const K& GetKey()   const { return reinterpret_cast<const K&>(Key); }
            V& GetValue() { return reinterpret_cast<V&>(Value); }
            const V& GetValue() const { return reinterpret_cast<const V&>(Value); }
        };

        static_assert(sizeof(K) == sizeof(typename Slot::KStorage), "Mismatch in key storage size");
        static_assert(sizeof(V) == sizeof(typename Slot::VStorage), "Mismatch in value storage size");

    public:
        // ---- Iterators ----

        struct KVPair
        {
            K& Key;
            V& Value;
        };

        struct ConstKVPair
        {
            const K& Key;
            const V& Value;
        };

        struct Iterator
        {
            Slot* Current;
            Slot* End;

            KVPair operator*() const { return { Current->GetKey(), Current->GetValue() }; }

            Iterator& operator++()
            {
                do { ++Current; } while (Current < End && Current->State != SlotState::Occupied);
                return *this;
            }

            bool operator==(const Iterator& inRHS) const { return Current == inRHS.Current; }
            bool operator!=(const Iterator& inRHS) const { return Current != inRHS.Current; }
        };

        struct ConstIterator
        {
            const Slot* Current;
            const Slot* End;

            ConstKVPair operator*() const { return { Current->GetKey(), Current->GetValue() }; }

            ConstIterator& operator++()
            {
                do { ++Current; } while (Current < End && Current->State != SlotState::Occupied);
                return *this;
            }

            bool operator==(const ConstIterator& inRHS) const { return Current == inRHS.Current; }
            bool operator!=(const ConstIterator& inRHS) const { return Current != inRHS.Current; }
        };

        // ---- Constructors ----

        HashMap() = default;

        explicit HashMap(Arena* arena, size_type inCapacity)
            : m_Arena(arena), m_Capacity(inCapacity)
        {
            m_Slots = static_cast<Slot*>(m_Arena->Alloc(sizeof(Slot) * m_Capacity));
            for (size_type i = 0; i < m_Capacity; ++i)
                m_Slots[i].State = SlotState::Empty;
        }

        HashMap(const HashMap<K, V, Hasher>& inRHS)
            : m_Arena(inRHS.m_Arena), m_Capacity(inRHS.m_Capacity)
        {
            m_Slots = static_cast<Slot*>(m_Arena->Alloc(sizeof(Slot) * m_Capacity));
            for (size_type i = 0; i < m_Capacity; ++i)
            {
                m_Slots[i].State = inRHS.m_Slots[i].State;
                if (inRHS.m_Slots[i].State == SlotState::Occupied)
                {
                    new (&m_Slots[i].Key)   K(inRHS.m_Slots[i].GetKey());
                    new (&m_Slots[i].Value) V(inRHS.m_Slots[i].GetValue());
                }
            }
            m_Size = inRHS.m_Size;
        }

        HashMap(HashMap<K, V, Hasher>&& inRHS) noexcept
            : m_Arena(inRHS.m_Arena), m_Capacity(inRHS.m_Capacity),
            m_Size(inRHS.m_Size), m_Slots(inRHS.m_Slots)
        {
            inRHS.m_Arena = nullptr;
            inRHS.m_Capacity = 0;
            inRHS.m_Size = 0;
            inRHS.m_Slots = nullptr;
        }

        ~HashMap() { clear(); m_Slots = nullptr; }

        // ---- Capacity ----

        bool      empty()    const { return m_Size == 0; }
        size_type size()     const { return m_Size; }
        size_type capacity() const { return m_Capacity; }

        // ---- Modifiers ----

        void clear()
        {
            if (!m_Slots) return;
            for (size_type i = 0; i < m_Capacity; ++i)
            {
                if (m_Slots[i].State == SlotState::Occupied)
                {
                    if constexpr (!std::is_trivially_destructible_v<K>)
                        m_Slots[i].GetKey().~K();
                    if constexpr (!std::is_trivially_destructible_v<V>)
                        m_Slots[i].GetValue().~V();
                }
                m_Slots[i].State = SlotState::Empty;
            }
            m_Size = 0;
        }

        /// Insert or overwrite. Returns pointer to the stored value.
        V* insert(const K& inKey, const V& inValue)
        {
            HBL2_CORE_ASSERT(m_Size * 10 < m_Capacity * 7, "HashMap load factor exceeded 70% — increase capacity.");
            size_type idx = ProbeInsert(inKey);
            Slot& slot = m_Slots[idx];

            if (slot.State == SlotState::Occupied)
            {
                slot.GetValue() = inValue;
            }
            else
            {
                new (&slot.Key)   K(inKey);
                new (&slot.Value) V(inValue);
                slot.State = SlotState::Occupied;
                ++m_Size;
            }
            return &slot.GetValue();
        }

        /// Construct value in-place. Returns pointer to the stored value.
        template<typename... Args>
        V* emplace(const K& inKey, Args&&... inArgs)
        {
            HBL2_CORE_ASSERT(m_Size * 10 < m_Capacity * 7, "HashMap load factor exceeded 70% — increase capacity.");
            size_type idx = ProbeInsert(inKey);
            Slot& slot = m_Slots[idx];

            if (slot.State == SlotState::Occupied)
            {
                if constexpr (!std::is_trivially_destructible_v<V>)
                    slot.GetValue().~V();
                new (&slot.Value) V(std::forward<Args>(inArgs)...);
            }
            else
            {
                new (&slot.Key)   K(inKey);
                new (&slot.Value) V(std::forward<Args>(inArgs)...);
                slot.State = SlotState::Occupied;
                ++m_Size;
            }
            return &slot.GetValue();
        }

        /// Remove by key. Returns true if the key existed.
        bool remove(const K& inKey)
        {
            size_type idx = ProbeFind(inKey);
            if (idx == InvalidIndex) return false;

            Slot& slot = m_Slots[idx];
            if constexpr (!std::is_trivially_destructible_v<K>)
                slot.GetKey().~K();
            if constexpr (!std::is_trivially_destructible_v<V>)
                slot.GetValue().~V();

            slot.State = SlotState::Deleted;
            --m_Size;
            return true;
        }

        // ---- Lookup ----

        /// Returns pointer to value, or nullptr if not found.
        V* find(const K& inKey)
        {
            size_type idx = ProbeFind(inKey);
            return idx != InvalidIndex ? &m_Slots[idx].GetValue() : nullptr;
        }

        const V* find(const K& inKey) const
        {
            size_type idx = ProbeFind(inKey);
            return idx != InvalidIndex ? &m_Slots[idx].GetValue() : nullptr;
        }

        bool contains(const K& inKey) const
        {
            return ProbeFind(inKey) != InvalidIndex;
        }

        /// Returns the value for a key, inserting a default-constructed one if absent.
        V& operator[](const K& inKey)
        {
            size_type idx = ProbeFind(inKey);
            if (idx != InvalidIndex)
                return m_Slots[idx].GetValue();

            HBL2_CORE_ASSERT(m_Size * 10 < m_Capacity * 7, "HashMap load factor exceeded 70% — increase capacity.");
            idx = ProbeInsert(inKey);
            Slot& slot = m_Slots[idx];
            new (&slot.Key)   K(inKey);
            new (&slot.Value) V();
            slot.State = SlotState::Occupied;
            ++m_Size;
            return slot.GetValue();
        }

        // ---- Iterators ----

        Iterator begin()
        {
            Slot* p = m_Slots, * e = m_Slots + m_Capacity;
            while (p < e && p->State != SlotState::Occupied) ++p;
            return { p, e };
        }

        Iterator end()
        {
            Slot* e = m_Slots + m_Capacity;
            return { e, e };
        }

        ConstIterator begin() const
        {
            const Slot* p = m_Slots, * e = m_Slots + m_Capacity;
            while (p < e && p->State != SlotState::Occupied) ++p;
            return { p, e };
        }

        ConstIterator end() const
        {
            const Slot* e = m_Slots + m_Capacity;
            return { e, e };
        }

        // ---- Assignment ----

        HashMap<K, V, Hasher>& operator=(const HashMap<K, V, Hasher>& inRHS)
        {
            if (this == &inRHS) return *this;
            clear();
            m_Arena = inRHS.m_Arena;
            m_Capacity = inRHS.m_Capacity;
            m_Slots = static_cast<Slot*>(m_Arena->Alloc(sizeof(Slot) * m_Capacity));
            for (size_type i = 0; i < m_Capacity; ++i)
            {
                m_Slots[i].State = inRHS.m_Slots[i].State;
                if (inRHS.m_Slots[i].State == SlotState::Occupied)
                {
                    new (&m_Slots[i].Key)   K(inRHS.m_Slots[i].GetKey());
                    new (&m_Slots[i].Value) V(inRHS.m_Slots[i].GetValue());
                }
            }
            m_Size = inRHS.m_Size;
            return *this;
        }

        HashMap<K, V, Hasher>& operator=(HashMap<K, V, Hasher>&& inRHS) noexcept
        {
            if (this == &inRHS) return *this;
            clear();
            m_Arena = inRHS.m_Arena;
            m_Capacity = inRHS.m_Capacity;
            m_Size = inRHS.m_Size;
            m_Slots = inRHS.m_Slots;
            inRHS.m_Arena = nullptr;
            inRHS.m_Capacity = 0;
            inRHS.m_Size = 0;
            inRHS.m_Slots = nullptr;
            return *this;
        }

    private:
        static constexpr size_type InvalidIndex = ~size_type(0);

        /// Linear probe for an existing key. Returns InvalidIndex if not found.
        size_type ProbeFind(const K& inKey) const
        {
            if (!m_Slots || m_Capacity == 0) return InvalidIndex;
            size_type idx = size_type(Hasher{}(inKey) % m_Capacity);
            for (size_type i = 0; i < m_Capacity; ++i)
            {
                const Slot& slot = m_Slots[idx];
                if (slot.State == SlotState::Empty)
                    return InvalidIndex;                               // Gap = key can't be further
                if (slot.State == SlotState::Occupied && slot.GetKey() == inKey)
                    return idx;
                idx = (idx + 1) % m_Capacity;                         // Skip tombstones, keep going
            }
            return InvalidIndex;
        }

        /// Linear probe for the best slot to insert into.
        /// Returns the matching occupied slot (for overwrite), a tombstone slot, or the first empty slot.
        size_type ProbeInsert(const K& inKey) const
        {
            size_type idx = size_type(Hasher{}(inKey) % m_Capacity);
            size_type firstDeleted = InvalidIndex;
            for (size_type i = 0; i < m_Capacity; ++i)
            {
                const Slot& slot = m_Slots[idx];
                if (slot.State == SlotState::Empty)
                    return firstDeleted != InvalidIndex ? firstDeleted : idx;
                if (slot.State == SlotState::Deleted && firstDeleted == InvalidIndex)
                    firstDeleted = idx;
                if (slot.State == SlotState::Occupied && slot.GetKey() == inKey)
                    return idx;
                idx = (idx + 1) % m_Capacity;
            }
            HBL2_CORE_ASSERT(firstDeleted != InvalidIndex, "HashMap is completely full with no tombstones — internal error.");
            return firstDeleted;
        }

        size_type  m_Size = 0;
        size_type  m_Capacity = 0;
        Slot* m_Slots = nullptr;
        Arena* m_Arena = nullptr;
    };
}