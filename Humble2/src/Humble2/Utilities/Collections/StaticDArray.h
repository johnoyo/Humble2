#pragma once

#include "Base.h"

#include <cstdint>
#include <initializer_list>

namespace HBL2
{
	/// Simple variable length array backed by a fixed size buffer (Adapted from Jolt StaticArray)
	template <class T, uint32_t N>
	class StaticDArray
	{
	public:
		using value_type = T;
		using size_type = uint32_t;
		static constexpr uint32_t Capacity = N;

		/// Default constructor
		StaticDArray() = default;

		/// Constructor from initializer list
		explicit StaticDArray(std::initializer_list<T> inList)
		{
			HBL2_CORE_ASSERT(inList.size() <= N, "");
			for (const T& v : inList)
			{
				new (reinterpret_cast<T*>(&m_Elements[m_Size++])) T(v);
			}
		}

		/// Copy constructor
		StaticDArray(const StaticDArray<T, N>& inRHS)
		{
			while (m_Size < inRHS.m_Size)
			{
				new (&m_Elements[m_Size]) T(inRHS[m_Size]);
				++m_Size;
			}
		}

		/// Destruct all elements
		~StaticDArray()
		{
			if constexpr (!std::is_trivially_destructible<T>())
			{
				for (T* e = reinterpret_cast<T*>(m_Elements), *end = e + m_Size; e < end; ++e)
				{
					e->~T();
				}
			}
		}

		/// Destruct all elements and set length to zero
		void clear()
		{
			if constexpr (!std::is_trivially_destructible<T>())
			{
				for (T* e = reinterpret_cast<T*>(m_Elements), *end = e + m_Size; e < end; ++e)
				{
					e->~T();
				}
			}
			m_Size = 0;
		}

		/// Add element to the back of the array
		void push_back(const T& inElement)
		{
			HBL2_CORE_ASSERT(m_Size < N, "");
			new (&m_Elements[m_Size++]) T(inElement);
		}

		/// Construct element at the back of the array
		template <class... A>
		void emplace_back(A &&... inElement)
		{
			HBL2_CORE_ASSERT(m_Size < N, "");
			new (&m_Elements[m_Size++]) T(std::forward<A>(inElement)...);
		}

		/// Remove element from the back of the array
		void pop_back()
		{
			HBL2_CORE_ASSERT(m_Size > 0, "");
			reinterpret_cast<T&>(m_Elements[--m_Size]).~T();
		}

		/// Returns true if there are no elements in the array
		bool empty() const
		{
			return m_Size == 0;
		}

		/// Returns amount of elements in the array
		size_type size() const
		{
			return m_Size;
		}

		/// Returns maximum amount of elements the array can hold
		size_type capacity() const
		{
			return N;
		}

		/// Resize array to new length
		void resize(size_type inNewSize)
		{
			HBL2_CORE_ASSERT(inNewSize <= N, "");
			if constexpr (!std::is_trivially_constructible<T>())
			{
				for (T* element = reinterpret_cast<T*>(m_Elements) + m_Size, *element_end = reinterpret_cast<T*>(m_Elements) + inNewSize; element < element_end; ++element)
				{
					new (element) T;
				}
			}
			
			if constexpr (!std::is_trivially_destructible<T>())
			{
				for (T* element = reinterpret_cast<T*>(m_Elements) + inNewSize, *element_end = reinterpret_cast<T*>(m_Elements) + m_Size; element < element_end; ++element)
				{
					element->~T();
				}
			}

			m_Size = inNewSize;
		}

		using const_iterator = const T*;

		/// Iterators
		const_iterator begin() const
		{
			return reinterpret_cast<const T*>(m_Elements);
		}

		const_iterator end() const
		{
			return reinterpret_cast<const T*>(m_Elements + m_Size);
		}

		using iterator = T*;

		iterator begin()
		{
			return reinterpret_cast<T*>(m_Elements);
		}

		iterator end()
		{
			return reinterpret_cast<T*>(m_Elements + m_Size);
		}

		const T* data() const
		{
			return reinterpret_cast<const T*>(m_Elements);
		}

		T* data()
		{
			return reinterpret_cast<T*>(m_Elements);
		}

		/// Access element
		T& operator [] (size_type inIdx)
		{
			HBL2_CORE_ASSERT(inIdx < m_Size, "");
			return reinterpret_cast<T&>(m_Elements[inIdx]);
		}

		const T& operator [] (size_type inIdx) const
		{
			HBL2_CORE_ASSERT(inIdx < m_Size, "");
			return reinterpret_cast<const T&>(m_Elements[inIdx]);
		}

		/// Access element
		T& at(size_type inIdx)
		{
			HBL2_CORE_ASSERT(inIdx < m_Size, "");
			return reinterpret_cast<T&>(m_Elements[inIdx]);
		}

		const T& at(size_type inIdx) const
		{
			HBL2_CORE_ASSERT(inIdx < m_Size, "");
			return reinterpret_cast<const T&>(m_Elements[inIdx]);
		}

		/// First element in the array
		const T& front() const
		{
			HBL2_CORE_ASSERT(m_Size > 0, "");
			return reinterpret_cast<const T&>(m_Elements[0]);
		}

		T& front()
		{
			HBL2_CORE_ASSERT(m_Size > 0, "");
			return reinterpret_cast<T&>(m_Elements[0]);
		}

		/// Last element in the array
		const T& back() const
		{
			HBL2_CORE_ASSERT(m_Size > 0, "");
			return reinterpret_cast<const T&>(m_Elements[m_Size - 1]);
		}

		T& back()
		{
			HBL2_CORE_ASSERT(m_Size > 0, "");
			return reinterpret_cast<T&>(m_Elements[m_Size - 1]);
		}

		/// Remove one element from the array
		void erase(const_iterator inIter)
		{
			size_type p = size_type(inIter - begin());
			HBL2_CORE_ASSERT(p < m_Size, "");
			reinterpret_cast<T&>(m_Elements[p]).~T();
			if (p + 1 < m_Size)
			{
				std::memmove(m_Elements + p, m_Elements + p + 1, (m_Size - p - 1) * sizeof(T));
			}
			--m_Size;
		}

		/// Remove multiple element from the array
		void erase(const_iterator inBegin, const_iterator inEnd)
		{
			size_type p = size_type(inBegin - begin());
			size_type n = size_type(inEnd - inBegin);
			HBL2_CORE_ASSERT(inEnd <= end(), "");
			for (size_type i = 0; i < n; ++i)
			{
				reinterpret_cast<T&>(m_Elements[p + i]).~T();
			}

			if (p + n < m_Size)
			{
				std::memmove(m_Elements + p, m_Elements + p + n, (m_Size - p - n) * sizeof(T));
			}
			m_Size -= n;
		}

		/// Assignment operator
		StaticDArray<T, N>& operator=(const StaticDArray<T, N>& inRHS)
		{
			size_type rhs_size = inRHS.size();

			if (static_cast<const void*>(this) != static_cast<const void*>(&inRHS))
			{
				clear();

				while (m_Size < rhs_size)
				{
					new (&m_Elements[m_Size]) T(inRHS[m_Size]);
					++m_Size;
				}
			}

			return *this;
		}

		/// Assignment operator with static array of different max length
		template <uint32_t M>
		StaticDArray<T, N>& operator=(const StaticDArray<T, M>& inRHS)
		{
			size_type rhs_size = inRHS.size();
			HBL2_CORE_ASSERT(rhs_size <= N, "");

			if (static_cast<const void*>(this) != static_cast<const void*>(&inRHS))
			{
				clear();

				while (m_Size < rhs_size)
				{
					new (&m_Elements[m_Size]) T(inRHS[m_Size]);
					++m_Size;
				}
			}

			return *this;
		}

		/// Comparing arrays
		bool operator==(const StaticDArray<T, N>& inRHS) const
		{
			if (m_Size != inRHS.m_Size)
			{
				return false;
			}

			for (size_type i = 0; i < m_Size; ++i)
			{
				if (!(reinterpret_cast<const T&>(m_Elements[i]) == reinterpret_cast<const T&>(inRHS.m_Elements[i])))
				{
					return false;
				}
			}

			return true;
		}

		bool operator!=(const StaticDArray<T, N>& inRHS) const
		{
			if (m_Size != inRHS.m_Size)
			{
				return true;
			}

			for (size_type i = 0; i < m_Size; ++i)
			{
				if (reinterpret_cast<const T&>(m_Elements[i]) != reinterpret_cast<const T&>(inRHS.m_Elements[i]))
				{
					return true;
				}
			}

			return false;
		}

	protected:
		struct alignas(T) Storage
		{
			uint8_t m_Data[sizeof(T)];
		};

		static_assert(sizeof(T) == sizeof(Storage), "Mismatch in size");
		static_assert(alignof(T) == alignof(Storage), "Mismatch in alignment");

		size_type m_Size = 0;
		Storage m_Elements[N];
	};
}