#pragma once

#include "Core/Application.h"

#include <string>

namespace HBL2
{
	namespace FileDialogs
	{
		HBL2_API std::string OpenFile(const std::string& title, const std::string& defaultPath, const std::vector<std::string>& filters);
		HBL2_API std::string SaveFile(const std::string& title, const std::string& defaultPath, const std::vector<std::string>& filters);
	};
}