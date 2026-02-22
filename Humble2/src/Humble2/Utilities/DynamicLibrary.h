#pragma once

#include "Log.h"

#if defined(_WIN32)
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

#include <string>

namespace HBL2
{
	class DynamicLibrary
	{
	public:
		DynamicLibrary() = default;
		DynamicLibrary(const std::string& path)
		{
			Load(path);
		}

		void Load(const std::string& path)
		{
#if defined(_WIN32)
			m_Library = LoadLibraryA(path.c_str());
#else
			m_Library = dlopen(path.c_str(), RTLD_LAZY);
#endif
			if (!m_Library)
			{
				HBL2_CORE_ERROR("Failed to load library: {}", path);
				m_Loaded = false;
				return;
			}

			m_Loaded = true;
		}

		void Free()
		{
			if (m_Library)
			{
#if defined(_WIN32)
				FreeLibrary((HMODULE)m_Library);
#else
				dlclose(m_Library);
#endif
			}

			m_Loaded = false;
		}

		template<typename Func>
		Func GetFunction(const std::string& name)
		{
#if defined(_WIN32)
			return reinterpret_cast<Func>(GetProcAddress((HMODULE)m_Library, name.c_str()));
#else
			return reinterpret_cast<Func>(dlsym(m_Library, name.c_str()));
#endif
		}

		inline const bool IsLoaded() const { return m_Loaded; }

	private:
		void* m_Library = nullptr;
		bool m_Loaded = false;
	};
}