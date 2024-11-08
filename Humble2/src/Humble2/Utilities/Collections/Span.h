#pragma once

#include <vector>
#include <initializer_list>

namespace HBL2
{
	template<typename T>
	class Span
	{
	public:
		Span() = default;
		Span(std::initializer_list<T> initializer_list) : Data(initializer_list.begin()), Size(initializer_list.size()) {}
		Span(T* data, size_t size) : Data(data), Size(size) {}
		Span(std::vector<T>& list) : Data(list.data()), Size(list.size()) {}

		template<size_t N>
		Span(T(&array)[N]) : Data(array), Size(sizeof(array) / sizeof(T)) {}

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