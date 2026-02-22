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
		};

		static BuildEngine* Instance;

		void Initialize();
		void ShutDown();

		virtual bool Build() = 0;
		virtual bool RunRuntime(Configuration configuration) = 0;
		virtual bool BuildRuntime(Configuration configuration) = 0;
		void Recompile();
		bool Exists();

		Handle<Asset> CreateSystemFile(const std::filesystem::path& currentDir, const std::string& systemName);
		Handle<Asset> CreateComponentFile(const std::filesystem::path& currentDir, const std::string& componentName);
		Handle<Asset> CreateHelperScriptFile(const std::filesystem::path& currentDir, const std::string& scriptName);

		void RegisterSystem(const std::string& name, Scene* ctx);
		void RegisterComponent(const std::string& name, Scene* ctx);

		void LoadBuild();
		void LoadBuild(const std::string& path);
		void UnloadBuild(Scene* ctx);

		const std::filesystem::path GetUnityBuildPath() const;
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
		const std::string m_UnityBuildSource = R"({ComponentIncludes}

{HelperScriptIncludes}

{SystemIncludes}
)";
		std::string m_UnityBuildSourceFinal;
	};
}