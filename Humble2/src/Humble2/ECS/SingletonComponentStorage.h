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

		virtual void* Add(Entity e) override
		{
			if (m_Entity == Entity::Null)
			{
				m_Entity = e;
				m_Component = T{};
				m_Mask.set(e.Idx);
				return &m_Component;
			}

			return nullptr;
		}

		virtual void Remove(Entity e) override
		{
			if (m_Entity == e)
			{
				if constexpr (!std::is_trivially_destructible_v<T>)
				{
					m_Component.~T();
				}

				m_Mask.clear(e.Idx);
				m_Entity = Entity::Null;
			}
		}

		virtual void* Get(Entity e) override
		{
			if (m_Entity == e)
			{
				return &m_Component;
			}

			return nullptr;
		}

		virtual bool Has(Entity e) const override
		{
			if (m_Entity == e)
			{
				return true;
			}
			return false;
		}

		virtual void Iterate(StaticFunction<void(void*), 128>&& func) override
		{
			if (m_Entity != Entity::Null)
			{
				func((void*)&m_Component);
			}
		}

		virtual void Clear() override
		{
			m_Mask.destroy();
			m_Entity = Entity::Null;
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
		Entity m_Entity = Entity::Null;
	};
}