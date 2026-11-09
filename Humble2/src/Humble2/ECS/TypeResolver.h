#pragma once

#include "Utilities/Collections/FixedArray.h"
#include "Utilities/Collections/FixedHashMap.h"

#include <cstdint>
#include <typeindex>

namespace HBL2
{
	struct TypeResolver
	{
	public:
		TypeResolver() = default;

		void Initialize(Arena* arena, uint32_t maxComponents)
		{
			m_TypeMap = FixedHashMap<std::type_index, uint32_t>(arena, maxComponents);
			m_FreeList = FixedArray<uint32_t>(arena, maxComponents);
		}

		void Clear()
		{
			m_TypeMap.clear();
		}

		template<typename T>
		uint32_t Resolve() const
		{
			using U = std::remove_cv_t<std::remove_reference_t<T>>;
			auto it = m_TypeMap.find(std::type_index(typeid(U)));

			if (it == m_TypeMap.end())
			{
				const_cast<TypeResolver*>(this)->Register<U>();
				it = m_TypeMap.find(std::type_index(typeid(U)));
			}

			return it->second;
		}

		template<typename T>
		uint32_t Register()
		{
			using U = std::remove_cv_t<std::remove_reference_t<T>>;
			std::type_index key(typeid(U));
			if (auto it = m_TypeMap.find(key); it != m_TypeMap.end())
			{
				return it->second;
			}

			uint32_t id;

			if (!m_FreeList.empty())
			{
				id = m_FreeList.back();
				m_FreeList.pop_back();
			}
			else
			{
				id = m_Next++;
			}

			m_TypeMap[key] = id;
			return id;
		}

		void Deregister(std::string_view typeName)
		{
			for (auto& [key, value] : m_TypeMap)
			{
				if (key.name() == typeName)
				{
					m_FreeList.push_back(value);
					m_TypeMap.erase(key);
					return;
				}
			}
		}

		uint32_t Count() const { return m_Next.load(); }

	private:
		FixedHashMap<std::type_index, uint32_t> m_TypeMap;
		FixedArray<uint32_t> m_FreeList;
		std::atomic<uint32_t> m_Next = 0;
	};
}
