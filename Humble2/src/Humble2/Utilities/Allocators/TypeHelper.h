#pragma once

#include <type_traits>

namespace HBL2
{
	template<typename T>
	class TypeHelper
	{
	public:
		template<typename U = T> requires(std::is_trivial_v<U>)
		static void ConstructRange(T* ptr, size_t count)
		{
			std::memset(ptr, 0, sizeof(T) * count);
		}

		template<typename U = T> requires(std::is_trivial_v<U>)
		static void ConstructRange(T* ptr, size_t count, const T& value)
		{
			if constexpr (sizeof(T) == 1)
			{
				std::memset(ptr, *reinterpret_cast<const char*>(&value), count);
			}
			else
			{
				for (size_t i = 0; i < count; ++i)
				{
					std::memcpy(ptr + i, &value, sizeof(T));
				}
			}
		}

		template<typename U = T> requires(!std::is_trivial_v<U>)
		static void ConstructRange(T* ptr, size_t count)
		{
			for (size_t i = 0; i < count; ++i)
			{
				new (ptr + i) T();
			}
		}

		template<typename U = T> requires(!std::is_trivial_v<U>)
		static void ConstructRange(T* ptr, size_t count, const T& value)
		{
			for (size_t i = 0; i < count; ++i)
			{
				new (ptr + i) T(value);
			}
		}

		template<typename U = T> requires (std::is_trivially_destructible_v<U>)
		static typename void DestroyRange(T* ptr, size_t count)
		{
		}

		template<typename U = T> requires (!std::is_trivially_destructible_v<U>)
		static void DestroyRange(T* ptr, size_t count)
		{
			for (size_t i = 0; i < count; ++i)
			{
				ptr[i].~T();
			}
		}
	};
}