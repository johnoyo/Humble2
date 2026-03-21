#pragma once

#include "IComponentStorage.h"
#include "Utilities\Collections\Collections.h"

namespace HBL2
{
    template<typename T, size_t N>
    class SmallComponentStorage final : public IComponentStorage
    {
        static_assert(N <= 64, "SmallComponentStorage holds maximum 64 components!");

    public:
        SmallComponentStorage(uint32_t maxEntities, PoolReservation* reservation)
            : m_MaxEntities(maxEntities)
        {
            m_Mask.Initialize(maxEntities, reservation);

            size_t bytes = 0;
            bytes += N * sizeof(EntityRef);
            bytes += N * sizeof(T);

            m_Arena.Initialize(&Allocator::Arena, bytes, reservation);

            m_Entities = (EntityRef*)m_Arena.Alloc(N * sizeof(EntityRef));
            m_Components = (T*)m_Arena.Alloc(N * sizeof(T));
            m_Size = 0;
        }

        virtual void* Add(EntityRef e) override
        {
            if (m_Size < N)
            {
                m_Entities[m_Size] = e;
                m_Components[m_Size] = T{};
                m_Mask.set(e.Idx);
                return &m_Components[m_Size++];
            }
            return nullptr;
        }

        virtual void Remove(EntityRef e) override
        {
            for (uint32_t i = 0; i < m_Size; ++i)
            {
                if (m_Entities[i].Idx == e.Idx)
                {
                    uint32_t last = m_Size - 1;

                    if (i != last)
                    {
                        m_Components[i] = std::move(m_Components[last]);
                        m_Entities[i] = m_Entities[last];
                    }

                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        m_Components[last].~T();
                    }

                    m_Mask.clear(e.Idx);
                    m_Size--;
                    return;
                }
            }
        }

        virtual void* Get(EntityRef e) override
        {
            for (uint32_t i = 0; i < m_Size; ++i)
            {
                if (m_Entities[i].Idx == e.Idx)
                {
                    return &m_Components[i];
                }
            }

            return nullptr;
        }

        virtual bool Has(EntityRef e) const override
        {
            return m_Mask.test(e.Idx);
        }

        virtual void Iterate(StaticFunction<void(void*), 128>&& func) override
        {
            for (uint32_t i = 0; i < m_Size; ++i)
            {
                func((void*)&m_Components[i]);
            }
        }

        virtual void Clear() override
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (uint32_t i = 0; i < m_Size; ++i)
                {
                    m_Components[i].~T();
                }
            }

            m_Size = 0;
            m_Mask.destroy();
            m_Arena.Destroy();
        }

        virtual const std::type_info& TypeInfo() const override
        {
            return typeid(T);
        }

        inline T& GetDirect(uint32_t entityIdx)
        {
            for (uint32_t i = 0; i < m_Size; ++i)
            {
                if (m_Entities[i].Idx == (int32_t)entityIdx)
                {
                    return m_Components[i];
                }
            }
        }

    private:
        Arena m_Arena;
        T* m_Components = nullptr;
        EntityRef* m_Entities = nullptr;
        uint32_t m_MaxEntities = 0;
        uint32_t m_Size = 0;
    };
}