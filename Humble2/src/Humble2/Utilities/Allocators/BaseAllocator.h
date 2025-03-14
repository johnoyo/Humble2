#pragma once

namespace HBL2
{
	/// <summary>
	/// The interface that all the allocators implement.
	/// 
	/// Uses the Curiously Recurring Template Pattern (CRTP) is a C++ idiom where a base class uses a derived class as a template parameter.
	/// This allows the base class to call methods from the derived class without virtual function overhead.
	/// </summary>
	/// <typeparam name="TAllocator">The derived allocator type.</typeparam>
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

		virtual void Free() = 0;
		virtual void Invalidate() = 0;
	};
}