#pragma once

namespace HBL2
{
	template<typename T, size_t N>
	class StaticArray
	{
	public:
		T Data[N];

		T& operator[](size_t i) { return Data[i]; }
		const T& operator[](size_t i) { return Data[i]; }

		size_t Size() { return N; }

		T* begin() { return Data; }
		T* end() { return Data + N; }
		const T* begin() const { return Data; }
		const T* end() const { return Data + N; }
	};
}