#pragma once

#include "IComponentStorage.h"
#include "Utilities\Collections\FixedArray.h"

namespace HBL2
{
	template<typename T>
	class SparseComponentStorage final : public IComponentStorage
	{
	public:
		SparseComponentStorage(uint32_t maxEntities, PoolReservation* reservation)
		{
			Initialize(maxEntities, reservation);
		}

		virtual void Initialize(uint32_t maxEntities, PoolReservation* reservation) override
		{
			if (m_IsInitialized)
			{
				return;
			}

			m_Mask.Initialize(maxEntities, reservation);

			size_t bytes = 0;
			bytes += maxEntities * sizeof(uint32_t);
			bytes += maxEntities * sizeof(Entity);
			bytes += maxEntities * sizeof(T);

			m_Arena.Initialize(&Allocator::Arena, bytes, reservation);

			m_EntityToIndex = FixedArray<uint32_t>(&m_Arena, maxEntities);
			m_EntityToIndex.resize(maxEntities);

			std::fill(m_EntityToIndex.begin(), m_EntityToIndex.end(), uint32_t(-1));

			m_Entities = FixedArray<Entity>(&m_Arena, maxEntities);
			m_Packed = FixedArray<T>(&m_Arena, maxEntities);

			m_IsInitialized = true;
		}

		virtual void Clear() override
		{
			m_Mask.destroy();
			m_Packed.clear();
			m_Entities.clear();
			std::fill(m_EntityToIndex.begin(), m_EntityToIndex.end(), uint32_t(-1));

			m_Arena.Destroy();

			m_IsInitialized = false;
		}

		virtual void* Add(Entity e) override
		{
			uint32_t idx = m_Packed.size();
			m_Packed.push_back(T{});
			m_Entities.push_back(e);
			m_EntityToIndex[e.Idx] = idx;
			m_Mask.set(e.Idx);

			return &m_Packed[idx];
		}

		virtual void Remove(Entity e) override
		{
			uint32_t idx = m_EntityToIndex[e.Idx];
			uint32_t last = m_Packed.size() - 1;

			// Swap the entity that we want to remove with the last one.
			std::swap(m_Packed[idx], m_Packed[last]);
			std::swap(m_Entities[idx], m_Entities[last]);

			// Update the index.
			m_EntityToIndex[m_Entities[idx].Idx] = idx;

			// Now simply remove the last element.
			m_Packed.pop_back();
			m_Entities.pop_back();

			// Update the bitset so it shows that the enity does not have this component.
			m_Mask.clear(e.Idx);
		}

		virtual void* Get(Entity e) override
		{
			return &m_Packed[m_EntityToIndex[e.Idx]];
		}

		virtual bool Has(Entity e) const override
		{
			return m_Mask.test(e.Idx);
		}

		virtual void Iterate(StaticFunction<void(void*), 128>&& func) override
		{
			for (uint32_t i = 0; i < m_Packed.size(); ++i)
			{
				func((void*)&m_Packed[i]);
			}
		}	

		virtual const std::type_info& TypeInfo() const override
		{
			return typeid(T);
		}

		inline T& GetDirect(uint32_t entityIdx)
		{
			return m_Packed[m_EntityToIndex[entityIdx]];
		}

	private:
		Arena m_Arena;

		FixedArray<T> m_Packed;
		FixedArray<Entity> m_Entities;
		FixedArray<uint32_t> m_EntityToIndex;
	};
}