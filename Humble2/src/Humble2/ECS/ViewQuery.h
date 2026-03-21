#pragma once

#include "StorageInfo.h"
#include "ExcludeQuery.h"

#include "Scene\TypeResolver.h"
#include "Utilities\Collections\StaticFunction.h"

namespace HBL2
{
    template<typename Component>
    class ViewQuery
    {
    public:
        ViewQuery(TypeResolver* typeResolver, int32_t* generations, StorageFor<Component>* storage, IComponentStorage** allStorages)
            : m_TypeResolver(typeResolver), m_Generations(generations), m_Storage(storage), m_AllStorages(allStorages)
        {

        }

        template<typename... ExcludeTypes>
        ExcludeQuery<IncludeWrapper<Component>, ExcludeWrapper<ExcludeTypes...>> Exclude()
        {
            return ExcludeQuery<IncludeWrapper<Component>, ExcludeWrapper<ExcludeTypes...>>(
                m_Generations,
                std::make_tuple(static_cast<StorageFor<std::remove_const_t<Component>>*>(EnsureStorage<std::remove_const_t<Component>>())),
                { EnsureStorage<std::remove_const_t<ExcludeTypes>>()... }
            );
        }

        template<typename Func>
        void ForEach(Func&& func)
        {
            using FuncT = std::decay_t<Func>;

            if constexpr (std::is_invocable_v<FuncT, EntityRef, Component&>)
            {
                m_Storage->Mask().forEach([&](uint32_t idx)
                {
                    EntityRef e{ (int32_t)idx, m_Generations[idx] };
                    func(e, m_Storage->GetDirect(idx));
                });
            }
            else
            {
                m_Storage->Iterate([&func](void* ptr)
                {
                    func(*static_cast<Component*>(ptr));
                });
            }
        }

    private:
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
        StorageFor<Component>* m_Storage = nullptr;
        int32_t* m_Generations = nullptr;

        TypeResolver* m_TypeResolver = nullptr;
        IComponentStorage** m_AllStorages = nullptr;
    };
}