#pragma once

#include "StaticArray.h"
#include "DynamicArray.h"

#include <vector>
#include <initializer_list>

namespace HBL2
{
	/// <summary>
	/// A lightweight, non-owning view of a contiguous sequence of elements, such as an array or a portion of a vector.
	/// It provides bounds-checked access without copying data, making it useful for efficient data manipulation and function parameter passing.
	/// </summary>
	/// <typeparam name="T">The type of the element to store in the array.</typeparam>
	template<typename T>
	class Span
	{
	public:
		Span() = default;
		Span(std::initializer_list<T> initializer_list) : m_Data(initializer_list.begin()), m_Size(initializer_list.size()) {}
		Span(T* data, size_t size) : m_Data(data), m_Size(size) {}
		Span(std::vector<T>& list) : m_Data(list.data()), m_Size(list.size()) {}
		Span(DynamicArray<T>& list) : m_Data(list.Data()), m_Size(list.Size()) {}

		template<typename TAllocator>
		Span(DynamicArray<T, TAllocator>& list) : m_Data(list.Data()), m_Size(list.Size()) {}

		template<size_t N>
		Span(T(&array)[N]) : m_Data(array), m_Size(sizeof(array) / sizeof(T)) {}

		template<size_t N>
		Span(StaticArray<T, N> array) : m_Data(array.Data()), m_Size(array.Size()) {}

		explicit Span(T* data) : m_Data(data), m_Size(1) {}
		explicit Span(T& data) : m_Data(&data), m_Size(1) {}

		operator std::initializer_list<T>() const { return std::initializer_list<T>(begin(), end()); }

		T* Data() { return m_Data; }
		const T* Data() const { return m_Data; }
		const size_t Size() const { return m_Size; }

		T* begin() { return m_Data; }
		T* end() { return m_Data + m_Size; }
		const T* begin() const { return m_Data; }
		const T* end() const { return m_Data + m_Size; }

	private:
		T* m_Data = nullptr;
		size_t m_Size = 0;
	};
}