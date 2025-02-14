#pragma once

#include <stdint.h>

namespace HBL2
{
	/// <summary>
	/// A fixed-size array that provides direct access to elements via indexing.
	/// It does not support dynamic resizing and is useful when the number of elements is known at compile time.
	/// </summary>
	/// <typeparam name="T">The type of the element to store in the array.</typeparam>
	/// <typeparam name="N">The size of the array.</typeparam>
	template<typename T, uint32_t N>
	class StaticArray
	{
	public:
		T& operator[](uint32_t i) { return m_Data[i]; }
		const T& operator[](uint32_t i) { return Data[i]; }

		/// <summary>
		/// Returns the number of elements in the array.
		/// </summary>
		/// <returns>The number of elements in the array.</returns>
		uint32_t Size() { return N; }

		/// <summary>
		/// Returns the raw pointer to the underlying data.
		/// </summary>
		/// <returns>The raw pointer to the underlying data.</returns>
		T* Data() { return m_Data; }

		T* begin() { return m_Data; }
		T* end() { return m_Data + N; }
		const T* begin() const { return m_Data; }
		const T* end() const { return m_Data + N; }

	private:
		T m_Data[N];
	};
}