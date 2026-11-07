#pragma once

#include "Utilities/Collections/ByteSpan.h"
#include "Utilities/Collections/FixedHashMap.h"

#include <filesystem>
#include <fstream>
#include <cstdint>

namespace HBL2
{
	class FileStreamWriter
	{
	public:
		FileStreamWriter(const std::filesystem::path& path);
		FileStreamWriter(const FileStreamWriter&) = delete;
		~FileStreamWriter();

		bool IsStreamGood() const;
		uint64_t GetStreamPosition();
		void SetStreamPosition(uint64_t position);
		bool WriteData(const char* data, size_t size);

		operator bool() const { return IsStreamGood(); }

		void WriteBytes(ByteSpan& bytes, bool writeSize = true);
		void WriteZero(uint64_t size);

		template<typename T>
		void WriteRaw(const T& type)
		{
			bool success = WriteData((char*)&type, sizeof(T));
			HBL2_CORE_ASSERT(success, "Failed to write raw data!");
		}

		template<typename T>
		void WriteObject(const T& obj)
		{
			// T::Serialize(this, obj);
		}

		template<typename Key, typename Value>
		void WriteMap(const FixedHashMap<Key, Value>& map, bool writeSize = true)
		{
			if (writeSize)
			{
				WriteRaw<uint32_t>((uint32_t)map.size());
			}

			for (const auto& [key, value] : map)
			{
				if constexpr (std::is_trivial<Key>())
				{
					WriteRaw<Key>(key);
				}
				else
				{
					WriteObject<Key>(key);
				}

				if constexpr (std::is_trivial<Value>())
				{
					WriteRaw<Value>(value);
				}
				else
				{
					WriteObject<Value>(value);
				}
			}
		}

	private:
		std::filesystem::path m_Path;
		std::ofstream m_Stream;
	};
}