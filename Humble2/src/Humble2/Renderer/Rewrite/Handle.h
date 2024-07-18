#pragma once

#include <stdint.h>

namespace HBL2
{
	template<typename T>
	class Handle
	{
	public:
		Handle() : m_ArrayIndex(0), m_GenerationalCounter(0) {}

		bool IsValid() const { return m_GenerationalCounter != 0; }
		bool operator==(const Handle<T>& other) const { return m_ArrayIndex == other.m_ArrayIndex && m_GenerationalCounter == other.m_GenerationalCounter; }
		bool operator!=(const Handle<T>& other) const { return m_ArrayIndex != other.m_ArrayIndex || m_GenerationalCounter != other.m_GenerationalCounter; }

		uint16_t Index() const { return m_ArrayIndex; }

	private:
		Handle(uint16_t arrayIndex, uint16_t generationalCounter) : m_ArrayIndex(arrayIndex), m_GenerationalCounter(generationalCounter) {}

		uint16_t m_ArrayIndex;
		uint16_t m_GenerationalCounter;

		template<typename U, typename H> friend class Pool;
	};
}