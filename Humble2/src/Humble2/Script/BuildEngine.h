#pragma once

#include "Humble2API.h"
#include "Scene\Scene.h"
#include "Utilities\DynamicLibrary.h"

#include <string>
#include <filesystem>

namespace HBL2
{

	class HBL2_API BuildEngine
	{
	public:
		enum class Configuration
		{
			Debug,
			Release,
			Distribution,
		};

		static BuildEngine* Instance;

		void Initialize();
		void ShutDown();

		virtual bool Build() = 0;
		virtual bool RunRuntime(Configuration configuration) = 0;
		virtual bool BuildRuntime(Configuration configuration) = 0;
		void Recompile();
		void HotReload(Handle<Scene> sceneHandle, const std::vector<std::string>& userComponentNames, const std::vector<std::string>& userSystemNames, std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>>& serializedUserComponents);
		bool Exists(Configuration configuration);
		void SetActiveConfiguration(Configuration configuration);
		Configuration GetActiveConfiguration() const;

		Handle<Asset> CreateSystemFile(const std::filesystem::path& currentDir, const std::string& systemName);
		Handle<Asset> CreateComponentFile(const std::filesystem::path& currentDir, const std::string& componentName);
		Handle<Asset> CreateHelperScriptFile(const std::filesystem::path& currentDir, const std::string& scriptName);

		void RegisterSystem(const std::string& name, Scene* ctx);
		void RegisterComponent(const std::string& name, Scene* ctx);

		void LoadBuild(Configuration config);
		void LoadBuild(const std::string& path);
		void UnloadBuild(Scene* ctx);

		const std::filesystem::path GetUnityBuildPath(Configuration config) const;
		std::string GetDefaultSystemCode(const std::string& systemName);
		std::string GetDefaultComponentCode(const std::string& componentName);
		std::string GetDefaultHelperScriptCode(const std::string& scriptName);

		entt::meta_any AddComponent(const std::string& name, Scene* ctx, Entity entity);
		entt::meta_any GetComponent(const std::string& name, Scene* ctx, Entity entity);
		void RemoveComponent(const std::string& name, Scene* ctx, Entity entity);
		bool HasComponent(const std::string& name, Scene* ctx, Entity entity);
		void ClearComponentStorage(const std::string& name, Scene* ctx);

		void SerializeComponents(const std::string& name, Scene* ctx, std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>>& data, bool cleanRegistry = true);
		void DeserializeComponents(const std::string& name, Scene* ctx, std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>>& data);

		std::string CleanComponentNameO1(const std::string& input);
		std::string CleanComponentNameO3(const std::string& input);

	protected:
		DynamicLibrary m_DynamicLibrary;
#ifdef DEBUG
		Configuration m_CurrentConfiguration = Configuration::Debug;
#else
		Configuration m_CurrentConfiguration = Configuration::Release;
#endif
		uint64_t m_RecompileCounter = 0;
		const std::string m_UnityBuildSource = R"({ComponentIncludes}

{HelperScriptIncludes}

{SystemIncludes}
)";
		std::string m_UnityBuildSourceFinal;
	};
}