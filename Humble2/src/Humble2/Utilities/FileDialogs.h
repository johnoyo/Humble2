#pragma once

#include "Core/Application.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <cstring>

namespace HBL2
{
	namespace FileDialogs
	{
		HBL2_API std::string OpenFile(const std::string& title, const std::string& defaultPath, const std::vector<std::string>& filters);
		HBL2_API std::string SaveFile(const std::string& title, const std::string& defaultPath, const std::vector<std::string>& filters);
	};

	namespace FileUtils
	{
		HBL2_API void CopyFolder(const std::filesystem::path& source, const std::filesystem::path& destination);

		HBL2_API std::string RelativePath(const std::filesystem::path& path, const std::filesystem::path& base);
	}
}