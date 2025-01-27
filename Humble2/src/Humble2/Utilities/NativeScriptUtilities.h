#pragma once

#include "Base.h"
#include "Scene\ISystem.h"

#include <Project/Project.h>
#include "Utilities\Random.h"

#include <entt.hpp>

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
			}
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

	private:
		void* m_Library = nullptr;
	};

	class HBL2_API NativeScriptUtilities
	{
	public:
		NativeScriptUtilities(const NativeScriptUtilities&) = delete;

		static NativeScriptUtilities& Get();

		static void Initialize();
		static void Shutdown();

		Handle<Asset> CreateSystemFile(const std::filesystem::path& currentDir, const std::string& systemName);

		void RegisterSystem(const std::string& name, Scene* ctx);

		void LoadUnityBuild(Scene* ctx);
		void LoadUnityBuild(Scene* ctx, const std::string& path);
		void UnloadUnityBuild(Scene* ctx);

		std::string GetDefaultSystemCode(const std::string& systemName);	
		std::string GetDefaultSolutionText(const std::string& systemName);	
		std::string GetDefaultProjectText(const std::string& projectIncludes);

		std::string GetDefaultComponentCode(const std::string& componentName);	

		void GenerateComponent(const std::string& componentName);
		void CompileComponent(const std::string& componentName);

		void LoadComponent(const std::string& path, Scene* ctx);
		entt::meta_any AddComponent(const std::string& name, Scene* ctx, entt::entity entity);
		entt::meta_any GetComponent(const std::string& name, Scene* ctx, entt::entity entity);
		bool HasComponent(const std::string& name, Scene* ctx, entt::entity entity);
		std::string CleanComponentName(const std::string& input);

	private:
		NativeScriptUtilities() = default;
		std::unordered_map<std::string, DynamicLibrary> m_DynamicLibraries;

		static NativeScriptUtilities* s_Instance;
	};
}