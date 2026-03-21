#pragma once

#include <cstdint>
#include <typeindex>
#include <unordered_map>

namespace HBL2
{
	struct TypeResolver
	{
	public:
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
			const uint32_t id = m_Next++;
			m_TypeMap.emplace(key, id);
			return id;
		}

		uint32_t Count() const { return m_Next.load(); }

	private:
		std::unordered_map<std::type_index, uint32_t> m_TypeMap{};
		std::atomic<uint32_t> m_Next = 0;
	};
}