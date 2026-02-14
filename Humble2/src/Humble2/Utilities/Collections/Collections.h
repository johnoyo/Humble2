#pragma once

#include "Core\Allocators.h"
#include "Utilities\Allocators\ArenaAllocator.h"

#include <vector>
#include <stack>

namespace HBL2
{
    // Dynamic Array
    template<typename T>
    using DArray = std::vector<T, ArenaAllocator<T>>;

    template<typename T>
    [[nodiscard]] inline DArray<T> MakeEmptyDArray()
    {
        return DArray<T>(0, ArenaAllocator<T>(&Allocator::DummyArena));
    }

    template<typename T>
    [[nodiscard]] inline DArray<T> MakeDArray(Arena& arena)
    {
        return DArray<T>(ArenaAllocator<T>(&arena));
    }

    template<typename T>
    [[nodiscard]] inline DArray<T> MakeDArray(Arena& arena, std::size_t reserveCount)
    {
        DArray<T> v{ ArenaAllocator<T>(&arena) };
        v.reserve(reserveCount);
        return v;
    }

    template<typename T>
    [[nodiscard]] inline DArray<T> MakeDArrayResized(Arena& arena, std::size_t resizeCount)
    {
        DArray<T> v{ ArenaAllocator<T>(&arena) };
        v.resize(resizeCount);
        return v;
    }

    template<typename T>
    [[nodiscard]] inline DArray<T> MakeDArray(ScratchArena& scratch)
    {
        return DArray<T>(ArenaAllocator<T>(&scratch));
    }

    template<typename T>
    [[nodiscard]] inline DArray<T> MakeDArray(ScratchArena& scratch, std::size_t reserveCount)
    {
        DArray<T> v{ ArenaAllocator<T>(&scratch) };
        v.reserve(reserveCount);
        return v;
    }

    template<typename T>
    [[nodiscard]] inline DArray<T> MakeDArrayResized(ScratchArena& scratch, std::size_t resizeCount)
    {
        DArray<T> v{ ArenaAllocator<T>(&scratch) };
        v.resize(resizeCount);
        return v;
    }

    // Hash Map
    template<typename K, typename V, typename Hash = std::hash<K>, typename KeyEq = std::equal_to<K>>
    using HMap = std::unordered_map<K, V, Hash, KeyEq, ArenaAllocator<std::pair<const K, V>>>;

    template<typename K, typename V, typename Hash = std::hash<K>, typename KeyEq = std::equal_to<K>>
    [[nodiscard]] inline HMap<K, V, Hash, KeyEq> MakeEmptyHMap()
    {
        using Alloc = ArenaAllocator<std::pair<const K, V>>;
        return HMap<K, V, Hash, KeyEq>(0, Hash{}, KeyEq{}, Alloc{ &Allocator::DummyArena });
    }

    template<typename K, typename V, typename Hash = std::hash<K>, typename KeyEq = std::equal_to<K>>
    [[nodiscard]] inline HMap<K, V, Hash, KeyEq> MakeHMap(Arena& arena)
    {
        using Alloc = ArenaAllocator<std::pair<const K, V>>;
        return HMap<K, V, Hash, KeyEq>(0, Hash{}, KeyEq{}, Alloc{ &arena });
    }

    template<typename K, typename V, typename Hash = std::hash<K>, typename KeyEq = std::equal_to<K>>
    [[nodiscard]] inline HMap<K, V, Hash, KeyEq> MakeHMap(Arena& arena, std::size_t reserveCount)
    {
        using Alloc = ArenaAllocator<std::pair<const K, V>>;
        HMap<K, V, Hash, KeyEq> m{ 0, Hash{}, KeyEq{}, Alloc{ &arena } };
        m.reserve(reserveCount); // big deal for arena allocators
        return m;
    }

    template<typename K, typename V, typename Hash = std::hash<K>, typename KeyEq = std::equal_to<K>>
    [[nodiscard]] inline HMap<K, V, Hash, KeyEq> MakeHMap(ScratchArena& scratch)
    {
        using Alloc = ArenaAllocator<std::pair<const K, V>>;
        return HMap<K, V, Hash, KeyEq>(0, Hash{}, KeyEq{}, Alloc{ scratch.GetArena() });
    }

    template<typename K, typename V, typename Hash = std::hash<K>, typename KeyEq = std::equal_to<K>>
    [[nodiscard]] inline HMap<K, V, Hash, KeyEq> MakeHMap(Arena& arena, std::size_t reserveCount, float maxLoadFactor)
    {
        using Alloc = ArenaAllocator<std::pair<const K, V>>;
        HMap<K, V, Hash, KeyEq> m(0, Hash{}, KeyEq{}, Alloc{ &arena });
        m.max_load_factor(maxLoadFactor);
        m.reserve(reserveCount);
        return m;
    }

    template<typename K, typename V, typename Hash = std::hash<K>, typename KeyEq = std::equal_to<K>>
    [[nodiscard]] inline HMap<K, V, Hash, KeyEq> MakeHMapWithBuckets(Arena& arena, std::size_t expectedElements, float maxLoadFactor)
    {
        using Alloc = ArenaAllocator<std::pair<const K, V>>;
        HMap<K, V, Hash, KeyEq> m(0, Hash{}, KeyEq{}, Alloc{ &arena });

        m.max_load_factor(maxLoadFactor);

        const std::size_t buckets = static_cast<std::size_t>(std::ceil(expectedElements / maxLoadFactor));

        m.rehash(buckets);
        return m;
    }

    // Stack
    template<typename T>
    using Stack = std::stack<T, ArenaAllocator<T>>;

    template<typename T>
    [[nodiscard]] inline Stack<T> MakeEmptyStack()
    {
        return Stack<T>(0, ArenaAllocator<T>(&Allocator::DummyArena));
    }

    template<typename T>
    [[nodiscard]] inline Stack<T> MakeStack(Arena& arena)
    {
        return Stack<T>(ArenaAllocator<T>(&arena));
    }

    template<typename T>
    [[nodiscard]] inline Stack<T> MakeStack(Arena& arena, std::size_t reserveCount)
    {
        Stack<T> v{ ArenaAllocator<T>(&arena) };
        v.reserve(reserveCount);
        return v;
    }

    template<typename T>
    [[nodiscard]] inline Stack<T> MakeStack(ScratchArena& scratch)
    {
        return Stack<T>(ArenaAllocator<T>(&scratch));
    }

    // String
    using String = std::basic_string<char, std::char_traits<char>, HBL2::ArenaAllocator<char>>;

    [[nodiscard]] inline String MakeEmptyString()
    {
        return String(0, '\0', HBL2::ArenaAllocator<char>(&Allocator::DummyArena));
    }

    [[nodiscard]] inline String MakeString(HBL2::Arena& arena)
    {
        return String{ HBL2::ArenaAllocator<char>(&arena) };
    }

    [[nodiscard]] inline String MakeString(HBL2::Arena& arena, std::size_t reserveCount)
    {
        String s{ HBL2::ArenaAllocator<char>(&arena) };
        s.reserve(reserveCount);
        return s;
    }

    [[nodiscard]] inline String MakeStringResized(HBL2::Arena& arena, std::size_t resizeCount)
    {
        String s{ HBL2::ArenaAllocator<char>(&arena) };
        s.resize(resizeCount);
        return s;
    }

    [[nodiscard]] inline String MakeString(HBL2::ScratchArena& scratch)
    {
        return String{ HBL2::ArenaAllocator<char>(&scratch) };
    }

    [[nodiscard]] inline String MakeString(HBL2::Arena& arena, std::string_view sv)
    {
        String s{ HBL2::ArenaAllocator<char>(&arena) };
        s.assign(sv.data(), sv.size());
        return s;
    }

    [[nodiscard]] inline String MakeString(HBL2::Arena& arena, const char* cstr)
    {
        String s{ HBL2::ArenaAllocator<char>(&arena) };
        s.assign(cstr);
        return s;
    }
}
