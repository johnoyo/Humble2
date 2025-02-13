#pragma once

#include "BaseAllocator.h"

#include <type_traits>

namespace HBL2
{
	class StandardAllocator final : public BaseAllocator<StandardAllocator>
	{
	public:
		template<typename T>
		T* Allocate(uint64_t size = sizeof(T))
		{
			T* data = (T*)operator new(size);
			memset(data, 0, size);
			return data;
		}

		template<typename T>
		void Deallocate(T* object)
		{
			if constexpr (std::is_array_v<T>)
			{
				delete[] object;
			}
			else
			{
				delete object;
			}
		}

		virtual void Invalidate() override
		{
		}

		virtual void Free() override
		{
		}
	};
}