#pragma once

#include "IComponentStorage.h"

namespace HBL2
{
	template<typename T>
	class DenseComponentStorage final : public IComponentStorage
	{
	public:
		DenseComponentStorage(uint32_t maxEntities, PoolReservation* reservation)
		{
			Initialize(maxEntities, reservation);
		}

		virtual void Initialize(uint32_t maxEntities, PoolReservation* reservation) override
		{
			if (m_IsInitialized)
			{
				return;
			}

			m_MaxEntities = maxEntities;
			m_Mask.Initialize(maxEntities, reservation);

			size_t bytes = maxEntities * sizeof(T);
			m_Arena.Initialize(&Allocator::Arena, bytes, reservation);
			m_Components = (T*)m_Arena.Alloc(m_MaxEntities * sizeof(T));

			m_IsInitialized = true;
		}

		virtual void Clear() override
		{
			if constexpr (!std::is_trivially_destructible_v<T>)
			{
				for (uint32_t i = 0; i < m_MaxEntities; ++i)
				{
					if (m_Mask.test(i))
					{
						m_Components[i].~T();
					}
				}
			}

			m_Mask.destroy();
			m_Components = nullptr;

			m_Arena.Destroy();

			m_IsInitialized = false;
		}

		virtual void* Add(Entity e) override
		{
			new (&m_Components[e.Idx]) T();
			m_Mask.set(e.Idx);

			return &m_Components[e.Idx];
		}

		virtual void Remove(Entity e) override
		{
			if constexpr (!std::is_trivially_destructible_v<T>)
			{
				m_Components[e.Idx].~T();
			}

			m_Mask.clear(e.Idx);
		}

		virtual void* Get(Entity e) override
		{
			return &m_Components[e.Idx];
		}

		virtual bool Has(Entity e) const override
		{
			return m_Mask.test(e.Idx);
		}

		virtual void Iterate(StaticFunction<void(void*), 128>&& func) override
		{
			for (uint32_t i = 0; i < m_MaxEntities; ++i)
			{
				if (m_Mask.test(i))
				{
					func((void*)&m_Components[i]);
				}
			}
		}

		virtual const std::type_info& TypeInfo() const override
		{
			return typeid(T);
		}

		inline T& GetDirect(uint32_t entityIdx)
		{
			return m_Components[entityIdx];
		}

	private:
		Arena m_Arena;
		T* m_Components = nullptr;
		uint32_t m_MaxEntities = 0;
	};
}