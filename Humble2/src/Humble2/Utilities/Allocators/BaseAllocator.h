#pragma once

#include <cstdint>

namespace HBL2
{
	template <typename T>
	class Span;

	/**
	 * @brief The interface that all allocators implement.
	 *
	 * Uses the Curiously Recurring Template Pattern (CRTP), a C++ idiom where a base class
	 * uses a derived class as a template parameter. This allows the base class to call methods
	 * from the derived class without virtual function overhead.
	 *
	 * @tparam TAllocator The derived allocator type.
	 */
	template<typename TAllocator>
	class BaseAllocator
	{
	public:
		template<typename T>
		T* Allocate(uint64_t size = sizeof(T))
		{
			return static_cast<TAllocator*>(this)->Allocate<T>(size);
		}

		template<typename T>
		void Deallocate(T* object)
		{
			return static_cast<TAllocator*>(this)->Deallocate<T>(object);
		}

		virtual void Initialize(size_t sizeInBytes) = 0;
		virtual void Free() = 0;
		virtual void Invalidate() = 0;

		// Helpers
		template <typename T>
		inline T* AllocateObject()
		{
			constexpr bool isValidConstr = std::is_trivially_constructible_v<T> || std::is_default_constructible_v<T>;
			static_assert(isValidConstr, "T must be trivially or default constructible to be used with a MemoryArena");

			T* ptr = Allocate<T>(sizeof(T));

			new(ptr) T{};

			return ptr;
		}

		template <typename T>
		inline T* AllocateObjects(size_t numberOfObjects)
		{
			constexpr bool isValidConstr = std::is_trivially_constructible_v<T> || std::is_default_constructible_v<T>;
			static_assert(isValidConstr, "T must be trivially or default constructible to be used with a MemoryArena");

			T* ptr = Allocate<T>(numberOfObjects * sizeof(T));

			Span<T> objects = { ptr, numberOfObjects };

			for (auto& obj : objects)
			{
				new(&obj) T{};
			}

			return ptr;
		}
	};
}