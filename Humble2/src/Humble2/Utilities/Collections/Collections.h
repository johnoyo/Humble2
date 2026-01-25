#pragma once

#include "Utilities\Allocators\ArenaAllocator.h"

#include <vector>
#include <stack>

namespace HBL2
{
    // Dynamic Array
    template<typename T>
    using DArray = std::vector<T, ArenaAllocator<T>>;

    template<typename T>
    [[nodiscard]] DArray<T> MakeDArray(Arena& arena)
    {
        return DArray<T>(ArenaAllocator<T>(&arena));
    }

    template<typename T>
    [[nodiscard]] DArray<T> MakeDArray(Arena& arena, std::size_t reserveCount)
    {
        DArray<T> v{ ArenaAllocator<T>(&arena) };
        v.reserve(reserveCount);
        return v;
    }

    template<typename T>
    [[nodiscard]] DArray<T> MakeDArray(ScratchArena& scratch)
    {
        return DArray<T>(ArenaAllocator<T>(&scratch));
    }

    // Hash Map
    template<typename K, typename V, typename Hash = std::hash<K>, typename KeyEq = std::equal_to<K>>
    using HMap = std::unordered_map<K, V, Hash, KeyEq, ArenaAllocator<std::pair<const K, V>>>;

    template<typename K, typename V, typename Hash = std::hash<K>, typename KeyEq = std::equal_to<K>>
    [[nodiscard]] HMap<K, V, Hash, KeyEq> MakeHMap(Arena& arena)
    {
        using Alloc = ArenaAllocator<std::pair<const K, V>>;
        return HMap<K, V, Hash, KeyEq>(0, Hash{}, KeyEq{}, Alloc{ &arena });
    }

    template<typename K, typename V, typename Hash = std::hash<K>, typename KeyEq = std::equal_to<K>>
    [[nodiscard]] HMap<K, V, Hash, KeyEq> MakeHMap(Arena& arena, std::size_t reserveCount)
    {
        using Alloc = ArenaAllocator<std::pair<const K, V>>;
        HMap<K, V, Hash, KeyEq> m{ 0, Hash{}, KeyEq{}, Alloc{ &arena } };
        m.reserve(reserveCount); // big deal for arena allocators
        return m;
    }

    template<typename K, typename V, typename Hash = std::hash<K>, typename KeyEq = std::equal_to<K>>
    [[nodiscard]] HMap<K, V, Hash, KeyEq> MakeHMap(ScratchArena& scratch)
    {
        using Alloc = ArenaAllocator<std::pair<const K, V>>;
        return HMap<K, V, Hash, KeyEq>(0, Hash{}, KeyEq{}, Alloc{ scratch.GetArena() });
    }

    template<typename K, typename V, typename Hash = std::hash<K>, typename KeyEq = std::equal_to<K>>
    [[nodiscard]] HMap<K, V, Hash, KeyEq> MakeHMap(Arena& arena, std::size_t reserveCount, float maxLoadFactor)
    {
        using Alloc = ArenaAllocator<std::pair<const K, V>>;
        HMap<K, V, Hash, KeyEq> m(0, Hash{}, KeyEq{}, Alloc{ &arena });
        m.max_load_factor(maxLoadFactor);
        m.reserve(reserveCount);
        return m;
    }

    // Stack
    template<typename T>
    using Stack = std::stack<T, ArenaAllocator<T>>;

    template<typename T>
    [[nodiscard]] Stack<T> MakeStack(Arena& arena)
    {
        return Stack<T>(ArenaAllocator<T>(&arena));
    }

    template<typename T>
    [[nodiscard]] Stack<T> MakeStack(Arena& arena, std::size_t reserveCount)
    {
        Stack<T> v{ ArenaAllocator<T>(&arena) };
        v.reserve(reserveCount);
        return v;
    }

    template<typename T>
    [[nodiscard]] Stack<T> MakeStack(ScratchArena& scratch)
    {
        return Stack<T>(ArenaAllocator<T>(&scratch));
    }
}
