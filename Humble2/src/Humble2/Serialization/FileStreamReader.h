#pragma once

#include "Utilities/Collections/ByteSpan.h"
#include "Utilities/Collections/FixedHashMap.h"

#include <filesystem>
#include <fstream>
#include <cstdint>

namespace HBL2
{
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

		void ReadBytes(ByteSpan& bytes, uint32_t size = 0);

		template<typename T>
		void ReadRaw(T& type)
		{
			bool success = ReadData((char*)&type, sizeof(T));
			HBL2_CORE_ASSERT(success, "Failed to read raw data!");
		}

		template<typename T>
		void ReadObject(T& obj)
		{
			T::Deserialize(this, obj);
		}

		template<typename Key, typename Value>
		void ReadMap(FixedHashMap<Key, Value>& map, uint32_t size = 0)
		{
			if (size == 0)
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