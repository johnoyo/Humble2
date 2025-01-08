#pragma once

#include "Core/Application.h"

#include <string>

namespace HBL2
{
	namespace FileDialogs
	{
		HBL2_API std::string OpenFile(const char* filter);
		HBL2_API std::string SaveFile(const char* filter);
	};
}