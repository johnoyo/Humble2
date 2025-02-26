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
		Handle<Asset> CreateComponentFile(const std::filesystem::path& currentDir, const std::string& componentName);
		Handle<Asset> CreateHelperScriptFile(const std::filesystem::path& currentDir, const std::string& scriptName);

		void RegisterSystem(const std::string& name, Scene* ctx);
		void RegisterComponent(const std::string& name, Scene* ctx);

		void LoadUnityBuild(Scene* ctx);
		void LoadUnityBuild(Scene* ctx, const std::string& path);
		void UnloadUnityBuild(Scene* ctx);
		
		const std::filesystem::path GetUnityBuildPath() const;
		std::string GetDefaultSystemCode(const std::string& systemName);	
		std::string GetDefaultSolutionText();	
		std::string GetDefaultProjectText(const std::string& projectIncludes);
		std::string GetDefaultComponentCode(const std::string& componentName);	
		std::string GetDefaultHelperScriptCode(const std::string& scriptName);

		entt::meta_any AddComponent(const std::string& name, Scene* ctx, entt::entity entity);
		entt::meta_any GetComponent(const std::string& name, Scene* ctx, entt::entity entity);
		void RemoveComponent(const std::string& name, Scene* ctx, entt::entity entity);
		bool HasComponent(const std::string& name, Scene* ctx, entt::entity entity);
		void ClearComponentStorage(const std::string& name, Scene* ctx);

		void SerializeComponents(const std::string& name, Scene* ctx, std::unordered_map<std::string, std::unordered_map<entt::entity, std::vector<std::byte>>>& data, bool cleanRegistry = true);
		void DeserializeComponents(const std::string& name, Scene* ctx, std::unordered_map<std::string, std::unordered_map<entt::entity, std::vector<std::byte>>>& data);

		std::string CleanComponentNameO1(const std::string& input);
		std::string CleanComponentNameO3(const std::string& input);

	private:
		NativeScriptUtilities() = default;
		std::unordered_map<std::string, DynamicLibrary> m_DynamicLibraries;

		static NativeScriptUtilities* s_Instance;
	};
}