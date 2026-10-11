#pragma once

#include "Humble2API.h"
#include "Scene/Scene.h"
#include "Renderer/SceneRenderer.h"
#include "Utilities/DynamicLibrary.h"

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
        
        virtual ~BuildEngine() = default;
        
        static BuildEngine* Create();

		void Initialize();
		void ShutDown();

		virtual bool Build() = 0;
		virtual bool RunRuntime(Configuration configuration) = 0;
		virtual bool BuildRuntime(Configuration configuration) = 0;
        virtual const std::filesystem::path GetUnityBuildPath(Configuration config) = 0;
        
        void Recompile();
        void HotReload(Handle<Scene> sceneHandle, const std::vector<std::string>& userComponentNames, const std::vector<std::string>& userSystemNames, Reflect::TypeEntry::ByteStorage& serializedUserComponents);
        bool Exists(Configuration configuration);
        void SetActiveConfiguration(Configuration configuration);
        Configuration GetActiveConfiguration() const;

        Handle<Asset> CreateSystemFile(const std::filesystem::path& currentDir, const std::string& systemName);
        Handle<Asset> CreateComponentFile(const std::filesystem::path& currentDir, const std::string& componentName);
        Handle<Asset> CreateHelperScriptFile(const std::filesystem::path& currentDir, const std::string& scriptName);
        Handle<Asset> CreateSceneRendererFile(const std::filesystem::path& currentDir, const std::string& sceneRendererName);

        void RegisterSystem(const std::string& name, Scene* ctx);
        void RegisterComponent(const std::string& name, Scene* ctx);
		SceneRenderer* RegisterSceneRenderer(const std::string& name);

        void LoadBuild(Configuration config);
        void LoadBuild(const std::string& path);
        void UnloadBuild(Scene* ctx);

		std::string GetDefaultSystemCode(const std::string& systemName);
		std::string GetDefaultComponentCode(const std::string& componentName);
		std::string GetDefaultHelperScriptCode(const std::string& scriptName);
		std::string GetDefaultSceneRendererCode(const std::string& sceneRendererName);

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

{SceneRendererIncludes}
)";
		std::string m_UnityBuildSourceFinal;
	};
}
