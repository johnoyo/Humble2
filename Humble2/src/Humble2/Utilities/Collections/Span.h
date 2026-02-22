#pragma once

#include "StaticArray.h"
#include "Collections.h"

#include <vector>
#include <initializer_list>

namespace HBL2
{
	/**
	 * @brief A lightweight, non-owning view of a contiguous sequence of elements.
	 *
	 * Span provides bounds-checked access to a range of elements without copying data.
	 * It is ideal for efficient data manipulation and function parameter passing.
	 *
	 * @tparam T The type of element in the span.
	 */
	template<typename T>
	class Span
	{
	public:
		Span() = default;
		Span(std::initializer_list<T> initializer_list) : m_Data(initializer_list.begin()), m_Size(initializer_list.size()) {}
		Span(T* data, size_t size) : m_Data(data), m_Size(size) {}

		Span(std::vector<T>& list) : m_Data(list.data()), m_Size(list.size()) {}
		template<typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
		Span(const std::vector<U>& list) : m_Data(list.data()), m_Size(list.size()) {}
		template<size_t N>
		Span(std::array<T, N>& array) : m_Data(array.data()), m_Size(array.size()) {}

		Span(DArray<T>& list) : m_Data(list.data()), m_Size(list.size()) {}
		template<typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
		Span(const DArray<U>& list) : m_Data(list.data()), m_Size(list.size()) {}

		template<size_t N>
		Span(T(&array)[N]) : m_Data(array), m_Size(sizeof(array) / sizeof(T)) {}

		template<size_t N>
		Span(StaticArray<T, N>& array) : m_Data(array.Data()), m_Size(array.Size()) {}

		explicit Span(T* data) : m_Data(data), m_Size(1) {}
		explicit Span(T& data) : m_Data(&data), m_Size(1) {}

		operator std::initializer_list<T>() const { return std::initializer_list<T>(begin(), end()); }

		T& operator[](uint32_t i) { return m_Data[i]; }
		const T& operator[](uint32_t i) const { return m_Data[i]; }

		T* Data() { return m_Data; }
		const T* Data() const { return m_Data; }
		const size_t Size() const { return m_Size; }

		T* begin() { return m_Data; }
		T* end() { return m_Data + m_Size; }
		const T* begin() const { return m_Data; }
		const T* end() const { return m_Data + m_Size; }

		T* rbegin() { return m_Data + m_Size - 1; }
        T* rend() { return m_Data - 1; }
        const T* rbegin() const { return m_Data + m_Size - 1; }
        const T* rend() const { return m_Data - 1; }

	private:
		T* m_Data = nullptr;
		size_t m_Size = 0;
	};
}