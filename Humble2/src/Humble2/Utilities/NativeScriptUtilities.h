#pragma once

#include "Base.h"
#include "Scene\ISystem.h"

#include "Utilities\Random.h"

#include <iostream>
#include <filesystem>
#include <cstdlib>

#if defined(_WIN32)
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

namespace HBL2
{
	class DynamicLibrary
	{
	public:
		explicit DynamicLibrary(const std::string& path)
		{
#if defined(_WIN32)
			library = LoadLibraryA(path.c_str());
#else
			library = dlopen(path.c_str(), RTLD_LAZY);
#endif
			if (!library)
			{
				HBL2_CORE_ERROR("Failed to load library: {}", path);
			}
		}

		~DynamicLibrary()
		{
			Free();
		}

		void Free()
		{
			if (library)
			{
#if defined(_WIN32)
				FreeLibrary((HMODULE)library);
#else
				dlclose(library);
#endif
			}
		}

		template<typename Func>
		Func GetFunction(const std::string& name)
		{
#if defined(_WIN32)
			return reinterpret_cast<Func>(GetProcAddress((HMODULE)library, name.c_str()));
#else
			return reinterpret_cast<Func>(dlsym(library, name.c_str()));
#endif
		}

	private:
		void* library = nullptr;
	};

	class HBL2_API NativeScriptUtilities
	{
	public:
		NativeScriptUtilities(const NativeScriptUtilities&) = delete;

		static NativeScriptUtilities& Get();

		static void Initialize();
		static void Shutdown();

		ISystem* LoadDLL(const std::string& dllPath);
		void DeleteDLLInstance(const std::string& dllName);

		std::string GetDefaultSystemCode(const std::string& systemName);		
		std::string GetDefaultSolutionText(const std::string& systemName);	
		std::string GetDefaultProjectText(const std::string& systemName);	

	private:
		NativeScriptUtilities() = default;

		std::unordered_map<std::string, HINSTANCE> m_DLLInstances;

		static NativeScriptUtilities* s_Instance;
	};
}