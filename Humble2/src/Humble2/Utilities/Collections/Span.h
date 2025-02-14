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
		Span(std::initializer_list<T> initializer_list) : Data(initializer_list.begin()), Size(initializer_list.size()) {}
		Span(T* data, size_t size) : Data(data), Size(size) {}
		Span(std::vector<T>& list) : Data(list.data()), Size(list.size()) {}
		Span(DynamicArray<T>& list) : Data(list.Data()), Size(list.Size()) {}

		template<size_t N>
		Span(T(&array)[N]) : Data(array), Size(sizeof(array) / sizeof(T)) {}

		template<size_t N>
		Span(StaticArray<T, N> array) : Data(array.Data()), Size(array.Size()) {}

		explicit Span(T* data) : Data(data), Size(1) {}
		explicit Span(T& data) : Data(&data), Size(1) {}

		operator std::initializer_list<T>() const
		{
			return std::initializer_list<T>(begin(), end());
		}

		T* begin() { return Data; }
		T* end() { return Data + Size; }
		const T* begin() const { return Data; }
		const T* end() const { return Data + Size; }

		T* Data = nullptr;
		size_t Size = 0;
	};
}