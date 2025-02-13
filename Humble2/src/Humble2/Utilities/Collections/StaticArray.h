#pragma once

#include <stdint.h>

namespace HBL2
{
	template<typename T, uint32_t N>
	class StaticArray
	{
	public:
		T& operator[](uint32_t i) { return m_Data[i]; }
		const T& operator[](uint32_t i) { return Data[i]; }

		uint32_t Size() { return N; }
		T* Data() { return m_Data; }

		T* begin() { return m_Data; }
		T* end() { return m_Data + N; }
		const T* begin() const { return m_Data; }
		const T* end() const { return m_Data + N; }

	private:
		T m_Data[N];
	};
}