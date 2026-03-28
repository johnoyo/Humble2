#pragma once

#include "IComponentStorage.h"
#include "ExcludeQuery.h"
#include "TwoLevelBitset.h"
#include "StorageInfo.h"
#include "TypeResolver.h"

#include "Utilities\Collections\Span.h"

namespace HBL2
{
    template<typename... Components>
    class FilterQuery
    {
    public:
        FilterQuery(TypeResolver* typeResolver, int32_t* genenarations, std::tuple<StorageFor<Components>*...> concreteStorages, Span<IComponentStorage*> allStorages)
            : m_TypeResolver(typeResolver), m_Generations(genenarations), m_AllStorages(allStorages), m_Storages(concreteStorages)
        {
        }

        template<typename... ExcludeTypes>
        ExcludeQuery<IncludeWrapper<Components...>, ExcludeWrapper<ExcludeTypes...>> Exclude()
        {
            return ExcludeQuery<IncludeWrapper<Components...>, ExcludeWrapper<ExcludeTypes...>> (m_Generations, m_Storages, { EnsureStorage<std::remove_const_t<ExcludeTypes>>()... });
        }

        template<typename Func>
        void ForEach(Func&& func)
        {
            ForEachImpl(std::forward<Func>(func), std::index_sequence_for<Components...>{});
        }

    private:
        template<typename Func, size_t... Indices>
        void ForEachImpl(Func&& func, std::index_sequence<Indices...>)
        {
            using FuncT = std::decay_t<Func>;

            if constexpr (std::is_invocable_v<FuncT, Entity, Components&...>)
            {
                TwoLevelBitset::forEachN(
                    [&](uint32_t idx)
                    {
                        Entity e{ (int32_t)idx, m_Generations[idx] };
                        func(e, std::get<Indices>(m_Storages)->GetDirect(idx)...);
                    },
                    std::get<Indices>(m_Storages)->Mask()...
                );
            }
            else
            {
                TwoLevelBitset::forEachN(
                    [&](uint32_t idx)
                    {
                        func(std::get<Indices>(m_Storages)->GetDirect(idx)...);
                    },
                    std::get<Indices>(m_Storages)->Mask()...
                );
            }
        }
        
        template<typename T>
        IComponentStorage* EnsureStorage()
        {
            uint32_t id = m_TypeResolver->Resolve<T>();

            if (!m_AllStorages[id])
            {
                return nullptr;
            }

            return m_AllStorages[id];
        }

    private:
        std::tuple<StorageFor<Components>*...> m_Storages;
        Span<IComponentStorage*> m_AllStorages;
        TypeResolver* m_TypeResolver = nullptr;
        int32_t* m_Generations = nullptr;
    };
}