#pragma once

namespace HBL2
{
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