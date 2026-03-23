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

		typedef void (*RegisterComponentFunc)();
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

		Reflect::TypeEntry::ByteStorage data;
		std::vector<std::string> userComponentNames;

		// Store all registered meta types.
		Reflect::ForEachRegisteredType([&](const Reflect::TypeEntry& entry)
		{
			if (entry.serialize)
			{
				userComponentNames.push_back(CleanComponentNameO3(std::string(entry.typeName)));
				entry.serialize(&activeScene->GetRegistry(), data, true);
			}
		});

		// Unload unity build dll.
		UnloadBuild(activeScene);

		// Build unity build source dll.
		Build();

		// Re-register systems.
		for (const auto& userSystemName : userSystemNames)
		{
			RegisterSystem(userSystemName, activeScene);
		}

		if (Context::Mode == Mode::Runtime)
		{
			for (ISystem* userSystem : activeScene->GetRuntimeSystems())
			{
				if (userSystem->GetType() == SystemType::User)
				{
					userSystem->SetState(SystemState::Play);
				}
			}
		}

		// Re-register the components.
		for (const auto& userComponentName : userComponentNames)
		{
			RegisterComponent(userComponentName, activeScene);
		}

		// Deserialize component data.
		Reflect::ForEachRegisteredType([&](const Reflect::TypeEntry& entry)
		{
			if (entry.deserialize)
			{
				entry.deserialize(&activeScene->GetRegistry(), data);
			}
		});
	}

	void BuildEngine::HotReload(Handle<Scene> sceneHandle, const std::vector<std::string>& userComponentNames, const std::vector<std::string>& userSystemNames, Reflect::TypeEntry::ByteStorage& serializedUserComponents)
	{
		Scene* activeScene = ResourceManager::Instance->GetScene(sceneHandle);

		// Unload unity build dll.
		UnloadBuild(activeScene);

		// Load new dll.
		LoadBuild(m_CurrentConfiguration);

		// Re-register systems.
		for (const auto& userSystemName : userSystemNames)
		{
			RegisterSystem(userSystemName, activeScene);
		}

		// Start user systems, since by default they start at idle.
		for (ISystem* userSystem : activeScene->GetRuntimeSystems())
		{
			if (userSystem->GetType() == SystemType::User)
			{
				userSystem->SetState(SystemState::Play);
			}
		}

		// Re-register the components.
		for (const auto& userComponentName : userComponentNames)
		{
			RegisterComponent(userComponentName, activeScene);
		}

		// Deserialize component data.
		Reflect::ForEachRegisteredType([&](const Reflect::TypeEntry& entry)
		{
			if (entry.deserialize)
			{
				entry.deserialize(&activeScene->GetRegistry(), serializedUserComponents);
			}
		});
	}

	bool BuildEngine::Exists(Configuration configuration)
	{
		const auto& path = GetUnityBuildPath(configuration);
		return std::filesystem::exists(path);
	}

	void BuildEngine::SetActiveConfiguration(Configuration configuration)
	{
		m_CurrentConfiguration = configuration;
	}

	BuildEngine::Configuration BuildEngine::GetActiveConfiguration() const
	{
		return m_CurrentConfiguration;
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

	const std::filesystem::path BuildEngine::GetUnityBuildPath(Configuration config) const
	{
		const std::string& projectName = Project::GetActive()->GetName();

		switch (config)
		{
		case Configuration::Debug: 
			return std::filesystem::path("assets") / "dlls" / "Debug-x86_64" / projectName / "UnityBuild.dll";
		case Configuration::Release: 
		case Configuration::Distribution: 
			return std::filesystem::path("assets") / "dlls" / "Release-x86_64" / projectName / "UnityBuild.dll";
		}

		return std::filesystem::path("");
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
struct {ComponentName}
{
    int Value = 1;

	// Member registration.
	static constexpr auto schema = HBL2::Reflect::Schema
	{
        HBL2::Reflect::Field{"Value", &{ComponentName}::Value}
    };
};

// Register component
REGISTER_HBL2_COMPONENT({ComponentName})
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
		const auto& path = GetUnityBuildPath(m_CurrentConfiguration);

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
		const auto& path = GetUnityBuildPath(m_CurrentConfiguration);

		// Retrieve if dll is not loaded.
		if (!m_DynamicLibrary.IsLoaded())
		{
			m_DynamicLibrary = DynamicLibrary(path.string());
		}

		// Retrieve function that registers the component from the dll.
		auto registerComponent = m_DynamicLibrary.GetFunction<RegisterComponentFunc>("RegisterComponent_" + name);

		// Register the component.
		registerComponent();
	}

	void BuildEngine::LoadBuild(Configuration config)
	{
		const auto& path = GetUnityBuildPath(config);
		LoadBuild(path.string());
	}

	void BuildEngine::LoadBuild(const std::string& path)
	{
		// Load new unity build dll.
		m_DynamicLibrary = DynamicLibrary(path);
	}

	void BuildEngine::UnloadBuild(Scene* ctx)
	{
		// Deregister systems.
		std::vector<ISystem*> systemsToBeDeregistered;

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
		
		// Clear user defined components.
		Reflect::ForEachRegisteredType([&](const Reflect::TypeEntry& entry)
		{
			if (entry.clearStorage)
			{
				entry.clearStorage(&ctx->GetRegistry());
			}

			// Deregister type from type resolver, since the dll will get freed and it will contain garbage.
			ctx->GetRegistry().GetTypeResolver().Deregister(entry.typeName);
		});

		// Clear reflection system.
		Reflect::GetRegistry().clear();

		// Free dll.
		if (m_DynamicLibrary.IsLoaded())
		{
			m_DynamicLibrary.Free();
		}
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