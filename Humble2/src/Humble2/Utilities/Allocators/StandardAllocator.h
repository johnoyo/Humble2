#pragma once

#include "BaseAllocator.h"

#include <type_traits>

namespace HBL2
{
	/**
	 * @brief The StandardAllocator class provides functionality to allocate and deallocate memory for objects.
	 *
	 * This allocator uses the standard C++ `new` and `delete` operators with zero-initialization.
	 */
	class StandardAllocator final : public BaseAllocator<StandardAllocator>
	{
	public:
		/**
		 * @brief Allocates memory for an object of type T.
		 *
		 * The allocated memory is zero-initialized.
		 *
		 * @tparam T The type of object to allocate.
		 * @param size The size of the memory to allocate (defaults to sizeof(T)).
		 * @return A pointer to the allocated and zero-initialized memory.
		 */
		template<typename T>
		T* Allocate(uint64_t size = sizeof(T))
		{
			T* data = (T*)operator new(size);
			std::memset(data, 0, size);
			return data;
		}

		/**
		 * @brief Deallocates previously allocated memory.
		 *
		 * @tparam T The type of object to deallocate.
		 * @param object A pointer to the object to be deallocated.
		 */
		template<typename T>
		void Deallocate(T* object)
		{
			operator delete(object);
		}

		/**
		 * @brief No-op for StandardAllocator as it does not pre allocate memory.
		 */
		virtual void Initialize(size_t sizeInBytes) override
		{
		}

		/**
		 * @brief No-op for StandardAllocator as it does not maintain state.
		 */
		virtual void Invalidate() override
		{
		}

		/**
		 * @brief No-op for StandardAllocator as it does not maintain state.
		 */
		virtual void Free() override
		{
		}
	};
}