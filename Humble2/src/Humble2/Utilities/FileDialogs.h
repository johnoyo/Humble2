#pragma once

#include "Core/Application.h"

#include <string>

namespace HBL2
{
	class FileDialogs
	{
	public:
		static std::string OpenFile(const char* filter);
		static std::string SaveFile(const char* filter);
	};
}