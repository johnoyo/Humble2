#pragma once

#include "StorageInfo.h"
#include "IComponentStorage.h"

#include "Utilities\Collections\StaticArray.h"

#include <utility>

namespace HBL2
{
    template<typename... T>
    struct IncludeWrapper {};

    template<typename... T>
    struct ExcludeWrapper {};

    template<typename Includes, typename Excludes>
    class ExcludeQuery;

    template<typename... IncludeTypes, typename... ExcludeTypes>
    class ExcludeQuery<IncludeWrapper<IncludeTypes...>, ExcludeWrapper<ExcludeTypes...>>
    {
    public:
        ExcludeQuery(int32_t* genenarations, std::tuple<StorageFor<IncludeTypes>*...> includes, StaticArray<IComponentStorage*, sizeof...(ExcludeTypes)> excludes)
            : m_Generations(genenarations), m_Include(includes), m_Exclude(excludes)
        {
        }

        template<typename Func>
        void ForEach(Func&& func)
        {
            ForEachImpl(std::forward<Func>(func), std::index_sequence_for<IncludeTypes...>{});
        }

    private:
        template<typename Func, size_t... Indices>
        void ForEachImpl(Func&& func, std::index_sequence<Indices...>)
        {
            using FuncT = std::decay_t<Func>;

            if constexpr (std::is_invocable_v<FuncT, Entity, IncludeTypes&...>)
            {
                TwoLevelBitset::forEachN(
                    [&](uint32_t idx)
                    {
                        for (size_t i = 0; i < sizeof...(ExcludeTypes); ++i)
                        {
                            if (!m_Exclude[i])
                            {
                                continue;
                            }

                            if (m_Exclude[i]->Mask().test(idx))
                            {
                                return;
                            }
                        }

                        Entity e{ (int32_t)idx, m_Generations[idx] };
                        func(e, std::get<Indices>(m_Include)->GetDirect(idx)...);
                    },
                    std::get<Indices>(m_Include)->Mask()...
                );
            }
            else
            {
                TwoLevelBitset::forEachN(
                    [&](uint32_t idx)
                    {
                        for (size_t i = 0; i < sizeof...(ExcludeTypes); ++i)
                        {
                            for (size_t i = 0; i < sizeof...(ExcludeTypes); ++i)
                            {
                                if (!m_Exclude[i])
                                {
                                    continue;
                                }

                                if (m_Exclude[i]->Mask().test(idx))
                                {
                                    return;
                                }
                            }
                        }

                        func(std::get<Indices>(m_Include)->GetDirect(idx)...);
                    },
                    std::get<Indices>(m_Include)->Mask()...
                );
            }
        }

    private:
        std::tuple<StorageFor<IncludeTypes>*...> m_Include;
        StaticArray<IComponentStorage*, sizeof...(ExcludeTypes)> m_Exclude;
        int32_t* m_Generations = nullptr;
    };
}