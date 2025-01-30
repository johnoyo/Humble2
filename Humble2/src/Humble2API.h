#pragma once

#ifdef HBL2_BUILD_DLL
	#define HBL2_API __declspec(dllexport)
#else
	#define HBL2_API __declspec(dllimport)
#endif