#pragma once

#include "Utilities/Collections/Span.h"
#include "Utilities/Collections/ByteSpan.h"
#include "Utilities/Collections/FixedHashMap.h"
#include "Utilities/Collections/StaticString.h"

#include <filesystem>
#include <fstream>
#include <cstdint>

namespace HBL2
{
	template<typename T>
	struct Serializer;

	class FileStreamReader
	{
	public:
		FileStreamReader(const std::filesystem::path& path);
		FileStreamReader(const FileStreamReader&) = delete;
		~FileStreamReader();

		bool IsStreamGood() const;
		uint64_t GetStreamPosition();
		void SetStreamPosition(uint64_t position);
		bool ReadData(char* destination, size_t size);

		size_t GetStreamLength();

		size_t ReadAsString(std::string& outString);
		uint32_t ReadString(char* dst, uint32_t maxLength);
		void ReadString(std::string& out);

		template<size_t N>
		void ReadString(StaticString<N>& out)
		{
			char buffer[N];
			const uint32_t length = ReadString(buffer, N - 1); // leave room for the null terminator
			out.assign(std::string_view(buffer, length));
		}

		void ReadBytes(ByteSpan& bytes, uint32_t size = UINT32_MAX);

		template<typename T>
		void ReadRaw(T& type)
		{
			bool success = ReadData((char*)&type, sizeof(T));
			HBL2_CORE_ASSERT(success, "Failed to read raw data!");
		}

		template<typename T>
		void ReadObject(T& obj)
		{
			Serializer<T>::Deserialize(this, obj);
		}

		template<typename T>
		void ReadSpan(Span<T> span, uint32_t size = UINT32_MAX)
		{
			if (size == UINT32_MAX)
			{
				ReadRaw<uint32_t>(size);
			}

			if (span.size() < size)
			{
				return;
			}

			for (uint32_t i = 0; i < size; i++)
			{
				if constexpr (std::is_trivial<T>())
				{
					ReadRaw<T>(span[i]);
				}
				else
				{
					ReadRaw<T>(span[i]);
				}
			}
		}

		template<typename Key, typename Value>
		void ReadMap(FixedHashMap<Key, Value>& map, uint32_t size = UINT32_MAX)
		{
			if (size == UINT32_MAX)
			{
				ReadRaw<uint32_t>(size);
			}

			for (uint32_t i = 0; i < size; i++)
			{
				Key key;
				if constexpr (std::is_trivial<Key>())
				{
					ReadRaw<Key>(key);
				}
				else
				{
					ReadObject<Key>(key);
				}

				if constexpr (std::is_trivial<Value>())
				{
					ReadRaw<Value>(map[key]);
				}
				else
				{
					ReadObject<Value>(map[key]);
				}
			}
		}

	private:
		std::filesystem::path m_Path;
		std::ifstream m_Stream;
	};
}