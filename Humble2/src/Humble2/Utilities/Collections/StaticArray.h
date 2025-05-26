#pragma once

#include <array>
#include <stdint.h>

namespace HBL2
{
	/**
	 * @brief A fixed-size stack array with direct element access.
	 *
	 * This array does not support dynamic resizing and is best used when the size
	 * is known at compile time.
	 *
	 * @tparam T The type of elements stored in the array.
	 * @tparam N The number of elements in the array.
	 */
	template<typename T, uint32_t N>
	class StaticArray
	{
	public:
		/**
		 * @brief Default construct elements (requires T to be default-constructible).
		 */
		StaticArray() = default;

		/**
		 * @brief Constructs from a C-style array.
		 * @param arr Reference to a C-style array of size N.
		 */
		StaticArray(const T(&arr)[N])
		{
			for (uint32_t i = 0; i < N; ++i)
			{
				m_Data[i] = arr[i];
			}
		}

		/**
		 * @brief Constructs from std::array.
		 * @param arr std::array of size N.
		 */
		StaticArray(const std::array<T, N>& arr)
		{
			for (uint32_t i = 0; i < N; ++i)
			{
				m_Data[i] = arr[i];
			}
		}

		/**
		 * @brief Constructs from initializer list. Size must match N.
		 * @param list Initializer list.
		 */
		StaticArray(std::initializer_list<T> list)
		{
			HBL2_CORE_ASSERT(list.size() == N, "Initializer list size must match StaticArray size");
			uint32_t i = 0;
			for (const auto& v : list)
			{
				m_Data[i++] = v;
			}
		}

		/**
		 * @brief Accesses the element at the specified index.
		 *
		 * @param i Index of the element.
		 * @return Reference to the element at the specified index.
		 */
		T& operator[](uint32_t i) { return m_Data[i]; }

		/**
		 * @brief Accesses the element at the specified index.
		 *
		 * @param i Index of the element.
		 * @return Reference to the element at the specified index.
		 */
		const T& operator[](uint32_t i) const { return m_Data[i]; }

		/**
		 * @brief Returns the number of elements in the array.
		 *
		 * @return The fixed size of the array.
		 */
		uint32_t Size() { return N; }

		/**
		 * @brief Returns a pointer to the underlying data.
		 *
		 * @return Pointer to the internal array.
		 */
		T* Data() { return m_Data; }

		T* begin() { return m_Data; }
		T* end() { return m_Data + N; }
		const T* begin() const { return m_Data; }
		const T* end() const { return m_Data + N; }

		T* rbegin() { return m_Data + N - 1; }
        T* rend() { return m_Data - 1; }
        const T* rbegin() const { return m_Data + N - 1; }
        const T* rend() const { return m_Data - 1; }

	private:
		T m_Data[N];
	};
}