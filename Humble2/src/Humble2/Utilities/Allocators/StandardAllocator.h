#pragma once

#include "BaseAllocator.h"

#include <type_traits>

namespace HBL2
{
	/// <summary>
	/// The StandardAllocator class provides functionality to allocate and deallocate memory for objects.
	/// It is a memory allocator that uses the standard C++ `new` and `delete` operators, with zero-initialization.
	/// </summary>
	class StandardAllocator final : public BaseAllocator<StandardAllocator>
	{
	public:
		/// <summary>
		/// Allocates memory for an object or array of objects of type T.
		/// The allocated memory is zero-initialized.
		/// </summary>
		/// <typeparam name="T">The type of the object to allocate.</typeparam>
		/// <param name="size">The size of the memory to allocate. Defaults to sizeof(T).</param>
		/// <returns>A pointer to the allocated memory.</returns>
		template<typename T>
		T* Allocate(uint64_t size = sizeof(T))
		{
			T* data = (T*)operator new(size);
			memset(data, 0, size);
			return data;
		}

		/// <summary>
		/// Deallocates previously allocated memory.
		/// </summary>
		/// <typeparam name="T">The type of the object to deallocate.</typeparam>
		/// <param name="object">A pointer to the object or array to be deallocated.</param>
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