#pragma once

#ifdef BUILD_DLL
	#define HBL2_API __declspec(dllexport)
#else
	#define HBL2_API 
#endif