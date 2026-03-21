#pragma once

#include "IComponentStorage.h"

namespace HBL2
{
	template<typename T>
	class SingletonComponentStorage final : public IComponentStorage
	{
	public:
		SingletonComponentStorage(uint32_t maxEntities, PoolReservation* reservation)
		{
			m_Mask.Initialize(maxEntities, reservation);
		}

		virtual void* Add(EntityRef e) override
		{
			if (m_Entity == EntityRef::Null)
			{
				m_Entity = e;
				m_Component = T{};
				m_Mask.set(e.Idx);
				return &m_Component;
			}

			return nullptr;
		}

		virtual void Remove(EntityRef e) override
		{
			if (m_Entity == e)
			{
				if constexpr (!std::is_trivially_destructible_v<T>)
				{
					m_Component.~T();
				}

				m_Mask.clear(e.Idx);
				m_Entity = EntityRef::Null;
			}
		}

		virtual void* Get(EntityRef e) override
		{
			if (m_Entity == e)
			{
				return &m_Component;
			}

			return nullptr;
		}

		virtual bool Has(EntityRef e) const override
		{
			if (m_Entity == e)
			{
				return true;
			}
			return false;
		}

		virtual void Iterate(StaticFunction<void(void*), 128>&& func) override
		{
			if (m_Entity != EntityRef::Null)
			{
				func((void*)&m_Component);
			}
		}

		virtual void Clear() override
		{
			m_Mask.destroy();
			m_Entity = EntityRef::Null;
		}

		virtual const std::type_info& TypeInfo() const override
		{
			return typeid(T);
		}

		inline T& GetDirect(uint32_t)
		{
			return m_Component;
		}

	private:
		T m_Component;
		EntityRef m_Entity = EntityRef::Null;
	};
}