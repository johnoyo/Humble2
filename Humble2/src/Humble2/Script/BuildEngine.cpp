#include "BuildEngine.h"

#include "Asset\AssetManager.h"
#include "Project\Project.h"
#include "Scene\ISystem.h"

namespace HBL2
{
	BuildEngine* BuildEngine::Instance = nullptr;

	extern "C"
	{
		typedef void (*RegisterSystemFunc)(Scene*);

		typedef const char* (*RegisterComponentFunc)(Scene*);

		typedef entt::meta_any(*AddComponentFunc)(Scene*, Entity);

		typedef entt::meta_any(*GetComponentFunc)(Scene*, Entity);

		typedef void (*RemoveComponentFunc)(Scene*, Entity);

		typedef bool (*HasComponentFunc)(Scene*, Entity);

		typedef void (*ClearComponentStorageFunc)(Scene*);

		typedef void (*SerializeComponentsFunc)(Scene*, std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>>&, bool);

		typedef void (*DeserializeComponentsFunc)(Scene*, std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>>&);
	}

	void BuildEngine::Initialize()
	{

	}

	void BuildEngine::ShutDown()
	{
		m_DynamicLibrary.Free();
	}

	void BuildEngine::Recompile()
	{
		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		std::vector<std::string> userSystemNames;

		// Store registered user system names.
		for (ISystem* userSystem : activeScene->GetRuntimeSystems())
		{
			if (userSystem->GetType() == SystemType::User)
			{
				userSystemNames.push_back(userSystem->Name);
			}
		}

		std::vector<std::string> userComponentNames;
		std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>> data;

		// Store all registered meta types.
		for (auto meta_type : entt::resolve(activeScene->GetMetaContext()))
		{
			std::string componentName = meta_type.second.info().name().data();
			componentName = CleanComponentNameO3(componentName);
			userComponentNames.push_back(componentName);

			SerializeComponents(componentName, activeScene, data);
		}

		// Unload unity build dll.
		UnloadBuild(activeScene);

		// Build unity build source dll.
		Build();

		// Re-register systems.
		for (const auto& userSystemName : userSystemNames)
		{
			RegisterSystem(userSystemName, activeScene);
		}

		// Re-register the components.
		for (const auto& userComponentName : userComponentNames)
		{
			RegisterComponent(userComponentName, activeScene);
			DeserializeComponents(userComponentName, activeScene, data);
		}
	}

	bool BuildEngine::Exists()
	{
		const auto& path = GetUnityBuildPath();
		return std::filesystem::exists(path);
	}

	Handle<Asset> BuildEngine::CreateSystemFile(const std::filesystem::path& currentDir, const std::string& systemName)
	{
		auto relativePath = std::filesystem::relative(currentDir / (systemName + ".h"), HBL2::Project::GetAssetDirectory());

		auto scriptAssetHandle = AssetManager::Instance->CreateAsset({
			.debugName = "script-asset",
			.filePath = relativePath,
			.type = AssetType::Script,
		});

		if (scriptAssetHandle.IsValid())
		{
			std::ofstream fout(HBL2::Project::GetAssetFileSystemPath(relativePath).string() + ".hblscript", 0);
			YAML::Emitter out;
			out << YAML::BeginMap;
			out << YAML::Key << "Script" << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "UUID" << YAML::Value << AssetManager::Instance->GetAssetMetadata(scriptAssetHandle)->UUID;
			out << YAML::Key << "Type" << YAML::Value << (uint32_t)ScriptType::SYSTEM;
			out << YAML::EndMap;
			out << YAML::EndMap;
			fout << out.c_str();
			fout.close();
		}

		std::ofstream fout(currentDir / (systemName + ".h"), 0);
		fout << GetDefaultSystemCode(systemName);
		fout.close();

		return scriptAssetHandle;
	}

	Handle<Asset> BuildEngine::CreateComponentFile(const std::filesystem::path& currentDir, const std::string& componentName)
	{
		auto relativePath = std::filesystem::relative(currentDir / (componentName + ".h"), HBL2::Project::GetAssetDirectory());

		auto scriptAssetHandle = AssetManager::Instance->CreateAsset({
			.debugName = "script-asset",
			.filePath = relativePath,
			.type = AssetType::Script,
			});

		if (scriptAssetHandle.IsValid())
		{
			std::ofstream fout(HBL2::Project::GetAssetFileSystemPath(relativePath).string() + ".hblscript", 0);
			YAML::Emitter out;
			out << YAML::BeginMap;
			out << YAML::Key << "Script" << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "UUID" << YAML::Value << AssetManager::Instance->GetAssetMetadata(scriptAssetHandle)->UUID;
			out << YAML::Key << "Type" << YAML::Value << (uint32_t)ScriptType::COMPONENT;
			out << YAML::EndMap;
			out << YAML::EndMap;
			fout << out.c_str();
			fout.close();
		}

		std::ofstream fout(currentDir / (componentName + ".h"), 0);
		fout << GetDefaultComponentCode(componentName);
		fout.close();

		return scriptAssetHandle;
	}

	Handle<Asset> BuildEngine::CreateHelperScriptFile(const std::filesystem::path& currentDir, const std::string& scriptName)
	{
		auto relativePath = std::filesystem::relative(currentDir / (scriptName + ".h"), HBL2::Project::GetAssetDirectory());

		auto scriptAssetHandle = AssetManager::Instance->CreateAsset({
			.debugName = "script-asset",
			.filePath = relativePath,
			.type = AssetType::Script,
			});

		if (scriptAssetHandle.IsValid())
		{
			std::ofstream fout(HBL2::Project::GetAssetFileSystemPath(relativePath).string() + ".hblscript", 0);
			YAML::Emitter out;
			out << YAML::BeginMap;
			out << YAML::Key << "Script" << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "UUID" << YAML::Value << AssetManager::Instance->GetAssetMetadata(scriptAssetHandle)->UUID;
			out << YAML::Key << "Type" << YAML::Value << (uint32_t)ScriptType::HELPER_SCRIPT;
			out << YAML::EndMap;
			out << YAML::EndMap;
			fout << out.c_str();
			fout.close();
		}

		std::ofstream fout(currentDir / (scriptName + ".h"), 0);
		fout << GetDefaultHelperScriptCode(scriptName);
		fout.close();

		return scriptAssetHandle;
	}

	const std::filesystem::path BuildEngine::GetUnityBuildPath() const
	{
		const std::string& projectName = Project::GetActive()->GetName();

#ifdef DEBUG
		return std::filesystem::path("assets") / "dlls" / "Debug-x86_64" / projectName / "UnityBuild.dll";
#else
		return std::filesystem::path("assets") / "dlls" / "Release-x86_64" / projectName / "UnityBuild.dll";
#endif
	}

	std::string BuildEngine::GetDefaultSystemCode(const std::string& systemName)
	{
		const std::string& placeholder = "{SystemName}";

		const std::string& systemCode = R"(#pragma once

#include "Humble2Core.h"

class {SystemName} final : public HBL2::ISystem
{
public:
	virtual void OnCreate() override
	{
	}

	virtual void OnUpdate(float ts) override
	{
	}
};

REGISTER_HBL2_SYSTEM({SystemName})
)";
		size_t pos = systemCode.find(placeholder);

		while (pos != std::string::npos)
		{
			((std::string&)systemCode).replace(pos, placeholder.length(), systemName);
			pos = systemCode.find(placeholder, pos + systemName.length());
		}

		return systemCode;
	}

	std::string BuildEngine::GetDefaultComponentCode(const std::string& componentName)
	{
		const std::string& placeholder = "{ComponentName}";

		const std::string& componentCode = R"(#pragma once

#include "Humble2Core.h"

// Just a POD struct
HBL2_COMPONENT({ComponentName},
{
    int Value = 1;
})

// Register members
REGISTER_HBL2_COMPONENT({ComponentName},
	HBL2_COMPONENT_MEMBER({ComponentName}, Value)
)
)";
		size_t pos = componentCode.find(placeholder);

		while (pos != std::string::npos)
		{
			((std::string&)componentCode).replace(pos, placeholder.length(), componentName);
			pos = componentCode.find(placeholder, pos + componentName.length());
		}

		return componentCode;
	}

	std::string BuildEngine::GetDefaultHelperScriptCode(const std::string& scriptName)
	{
		const std::string& placeholder = "{ScriptName}";

		const std::string& scriptCode = R"(#pragma once

#include "Humble2Core.h"

class {ScriptName}
{
	public:
    private:
};
)";
		size_t pos = scriptCode.find(placeholder);

		while (pos != std::string::npos)
		{
			((std::string&)scriptCode).replace(pos, placeholder.length(), scriptName);
			pos = scriptCode.find(placeholder, pos + scriptName.length());
		}

		return scriptCode;
	}

	void BuildEngine::RegisterSystem(const std::string& name, Scene* ctx)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve if dll is not loaded.
		if (!m_DynamicLibrary.IsLoaded())
		{
			m_DynamicLibrary = DynamicLibrary(path.string());
		}

		// Retrieve function that creates the system from the dll.
		auto registerSystem = m_DynamicLibrary.GetFunction<RegisterSystemFunc>("RegisterSystem_" + name);

		// Create the system
		registerSystem(ctx);
	}

	void BuildEngine::RegisterComponent(const std::string& name, Scene* ctx)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve if dll is not loaded.
		if (!m_DynamicLibrary.IsLoaded())
		{
			m_DynamicLibrary = DynamicLibrary(path.string());
		}

		// Retrieve function that registers the component from the dll.
		auto registerComponent = m_DynamicLibrary.GetFunction<RegisterComponentFunc>("RegisterComponent_" + name);

		// Register the component.
		const char* properName = registerComponent(ctx);
	}

	void BuildEngine::LoadBuild()
	{
		const auto& path = GetUnityBuildPath();
		LoadBuild(path.string());
	}

	void BuildEngine::LoadBuild(const std::string& path)
	{
		// Load new unity build dll.
		m_DynamicLibrary = DynamicLibrary(path);
	}

	void BuildEngine::UnloadBuild(Scene* ctx)
	{
		// Clear user defined components.
		for (auto meta_type : entt::resolve(ctx->GetMetaContext()))
		{
			const auto& alias = meta_type.second.info().name();

			if (alias.size() == 0 || alias.size() >= UINT32_MAX || alias.data() == nullptr)
			{
				continue;
			}

			const std::string& componentName = alias.data();

			const std::string& cleanedComponentName = CleanComponentNameO3(componentName);
			ClearComponentStorage(cleanedComponentName, ctx);
		}

		// Reset reflection system.
		entt::meta_reset(ctx->GetMetaContext());
		ctx->GetRegistry().compact();

		std::vector<ISystem*> systemsToBeDeregistered;

		// Deregister systems.
		for (ISystem* userSystem : ctx->GetRuntimeSystems())
		{
			if (userSystem->GetType() == SystemType::User)
			{
				systemsToBeDeregistered.push_back(userSystem);
			}
		}

		for (const auto& system : systemsToBeDeregistered)
		{
			ctx->DeregisterSystem(system);
		}

		systemsToBeDeregistered.clear();

		// Free dll.
		if (m_DynamicLibrary.IsLoaded())
		{
			m_DynamicLibrary.Free();
		}
	}

	entt::meta_any BuildEngine::AddComponent(const std::string& name, Scene* ctx, Entity entity)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve function that registers the component from the dll.
		auto addComponent = m_DynamicLibrary.GetFunction<AddComponentFunc>("AddComponent_" + name);

		// Register the component.
		return addComponent(ctx, entity);
	}

	entt::meta_any BuildEngine::GetComponent(const std::string& name, Scene* ctx, Entity entity)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve function that gets the component from the dll.
		auto getComponent = m_DynamicLibrary.GetFunction<GetComponentFunc>("GetComponent_" + name);

		// Register the component.
		return getComponent(ctx, entity);
	}

	void BuildEngine::RemoveComponent(const std::string& name, Scene* ctx, Entity entity)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve function that removes the component from the dll.
		auto removeComponent = m_DynamicLibrary.GetFunction<RemoveComponentFunc>("RemoveComponent_" + name);

		// Remove the component.
		removeComponent(ctx, entity);
	}

	bool BuildEngine::HasComponent(const std::string& name, Scene* ctx, Entity entity)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve function that checks if the entity has the component from the dll.
		auto hasComponent = m_DynamicLibrary.GetFunction<HasComponentFunc>("HasComponent_" + name);

		// Register the component.
		return hasComponent(ctx, entity);
	}

	void BuildEngine::ClearComponentStorage(const std::string& name, Scene* ctx)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve function that checks if the entity has the component from the dll.
		auto clearComponentStorage = m_DynamicLibrary.GetFunction<ClearComponentStorageFunc>("ClearComponentStorage_" + name);

		// Register the component.
		clearComponentStorage(ctx);
	}

	void BuildEngine::SerializeComponents(const std::string& name, Scene* ctx, std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>>& data, bool cleanRegistry)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve if dll is not loaded.
		if (!m_DynamicLibrary.IsLoaded())
		{
			m_DynamicLibrary = DynamicLibrary(path.string());
		}

		auto serializeComponents = m_DynamicLibrary.GetFunction<SerializeComponentsFunc>("SerializeComponents_" + name);

		serializeComponents(ctx, data, cleanRegistry);
	}

	void BuildEngine::DeserializeComponents(const std::string& name, Scene* ctx, std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>>& data)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve if dll is not loaded.
		if (!m_DynamicLibrary.IsLoaded())
		{
			m_DynamicLibrary = DynamicLibrary(path.string());
		}

		auto deserializeComponents = m_DynamicLibrary.GetFunction<DeserializeComponentsFunc>("DeserializeComponents_" + name);

		deserializeComponents(ctx, data);
	}

	std::string BuildEngine::CleanComponentNameO1(const std::string& input)
	{
		std::string output = input;

		// Find the '>' character and truncate the string if it exists
		size_t pos = output.find('>');
		if (pos != std::string::npos)
		{
			return output = output.substr(0, pos);
		}

		return output;
	}

	std::string BuildEngine::CleanComponentNameO3(const std::string& input)
	{
		std::string output = input;

		// Remove "struct " if it exists
		const std::string structPrefix = "struct ";
		size_t structPos = output.find(structPrefix);
		if (structPos != std::string::npos)
		{
			output.erase(structPos, structPrefix.length());
		}

		// Remove "class " if it exists
		const std::string classPrefix = "class ";
		size_t classPos = output.find(classPrefix);
		if (classPos != std::string::npos)
		{
			output.erase(classPos, classPrefix.length());
		}

		// TODO: Handle namespaces

		// Find the '>' character and truncate the string if it exists
		size_t pos1 = output.find('>');
		if (pos1 != std::string::npos)
		{
			output = output.substr(0, pos1);
		}

		// Find the '_' character and truncate the string if it exists
		size_t pos2 = output.find('_');
		if (pos2 != std::string::npos)
		{
			return output.substr(0, pos2);
		}

		return output;
	}
}