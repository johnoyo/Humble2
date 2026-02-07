#include "ProjectSerializer.h"

#include <fstream>
#include <yaml-cpp\yaml.h>

namespace HBL2
{
	ProjectSerializer::ProjectSerializer(Project* project)
		: m_Project(project)
	{
	}

	void ProjectSerializer::Serialize(const std::filesystem::path& filePath)
	{
		const auto& spec = m_Project->GetSpecification();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Project" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Name" << YAML::Value << spec.Name;
		out << YAML::Key << "StartingScene" << YAML::Value << spec.StartingScene.string();
		out << YAML::Key << "AssetDirectory" << YAML::Value << spec.AssetDirectory.string();
		out << YAML::Key << "ScriptDirectory" << YAML::Value << spec.ScriptDirectory.string();

		out << YAML::Key << "Renderer" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Type" << YAML::Value << (int)spec.Settings.Renderer;
		out << YAML::Key << "Editor API" << YAML::Value << (int)spec.Settings.EditorGraphicsAPI;
		out << YAML::Key << "Runtime API" << YAML::Value << (int)spec.Settings.RuntimeGraphicsAPI;
		out << YAML::EndMap;

		out << YAML::Key << "Physics2D" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Gravity Force" << YAML::Value << spec.Settings.GravityForce2D;
		out << YAML::Key << "Enable Debug Draw" << YAML::Value << spec.Settings.EnableDebugDraw2D;
		out << YAML::Key << "Implementation" << YAML::Value << (int)spec.Settings.Physics2DImpl;
		out << YAML::EndMap;

		out << YAML::Key << "Physics3D" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Gravity Force" << YAML::Value << spec.Settings.GravityForce2D;
		out << YAML::Key << "Enable Debug Draw" << YAML::Value << spec.Settings.EnableDebugDraw3D;
		out << YAML::Key << "Show Colliders" << YAML::Value << spec.Settings.ShowColliders3D;
		out << YAML::Key << "Show Bounding Boxes" << YAML::Value << spec.Settings.ShowBoundingBoxes3D;
		out << YAML::Key << "Implementation" << YAML::Value << (int)spec.Settings.Physics3DImpl;
		out << YAML::EndMap;

		out << YAML::Key << "Sound" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Implementation" << YAML::Value << (int)spec.Settings.SoundImpl;
		out << YAML::EndMap;

		out << YAML::Key << "Editor" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Multiple Viewports" << YAML::Value << spec.Settings.EditorMultipleViewports;
		out << YAML::EndMap;

		out << YAML::Key << "Advanced" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Max App Memory" << YAML::Value << spec.Settings.MaxAppMemory;
		out << YAML::Key << "Max UniformBuffer Memory" << YAML::Value << spec.Settings.MaxUniformBufferMemory;

		out << YAML::BeginMap;
		out << YAML::Key << "Resource Manager" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Textures Pool Size" << YAML::Value << spec.Settings.ResourceManagerSpec.Textures;
		out << YAML::Key << "Shaders Pool Size" << YAML::Value << spec.Settings.ResourceManagerSpec.Shaders;
		out << YAML::Key << "Buffers Pool Size" << YAML::Value << spec.Settings.ResourceManagerSpec.Buffers;
		out << YAML::Key << "BindGroups Pool Size" << YAML::Value << spec.Settings.ResourceManagerSpec.BindGroups;
		out << YAML::Key << "BindGroupLayouts Pool Size" << YAML::Value << spec.Settings.ResourceManagerSpec.BindGroupLayouts;
		out << YAML::Key << "FrameBuffers Pool Size" << YAML::Value << spec.Settings.ResourceManagerSpec.FrameBuffers;
		out << YAML::Key << "RenderPass Pool Size" << YAML::Value << spec.Settings.ResourceManagerSpec.RenderPass;
		out << YAML::Key << "RenderPassLayouts Pool Size" << YAML::Value << spec.Settings.ResourceManagerSpec.RenderPassLayouts;
		out << YAML::Key << "Meshes Pool Size" << YAML::Value << spec.Settings.ResourceManagerSpec.Meshes;
		out << YAML::Key << "Materials Pool Size" << YAML::Value << spec.Settings.ResourceManagerSpec.Materials;
		out << YAML::Key << "Scenes Pool Size" << YAML::Value << spec.Settings.ResourceManagerSpec.Scenes;
		out << YAML::Key << "Scripts Pool Size" << YAML::Value << spec.Settings.ResourceManagerSpec.Scripts;
		out << YAML::Key << "Sounds Pool Size" << YAML::Value << spec.Settings.ResourceManagerSpec.Sounds;
		out << YAML::Key << "Prefabs Pool Size" << YAML::Value << spec.Settings.ResourceManagerSpec.Prefabs;
		out << YAML::EndMap;
		out << YAML::EndMap;

		out << YAML::BeginMap;
		out << YAML::Key << "Asset Manager" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Assets Pool Size" << YAML::Value << spec.Settings.AssetManagerSpec.Assets;
		out << YAML::EndMap;
		out << YAML::EndMap;

		out << YAML::EndMap;

		out << YAML::EndMap;
		out << YAML::EndMap;

		std::ofstream fOut(filePath);
		fOut << out.c_str();
		fOut.close();
	}

	bool ProjectSerializer::Deserialize(const std::filesystem::path& filePath)
	{
		auto& spec = m_Project->GetSpecification();

		YAML::Node data;

		try
		{
			data = YAML::LoadFile(filePath.string());
		}
		catch (YAML::ParserException e)
		{
			HBL2_CORE_ERROR("Failed to load project: {0}\n {1}", filePath.string(), e.what());
			return false;
		}

		if (!data["Project"].IsDefined())
		{
			HBL2_CORE_ERROR("Project not found: {0}", filePath.string());
			return false;
		}

		spec.Name = data["Project"]["Name"].as<std::string>();
		spec.StartingScene = data["Project"]["StartingScene"].as<std::string>();
		spec.AssetDirectory = data["Project"]["AssetDirectory"].as<std::string>();
		spec.ScriptDirectory = data["Project"]["ScriptDirectory"].as<std::string>();

		if (!data["Project"]["Renderer"].IsDefined() ||	!data["Project"]["Physics2D"].IsDefined() || !data["Project"]["Physics3D"].IsDefined()
		 || !data["Project"]["Advanced"].IsDefined() || !data["Project"]["Editor"].IsDefined() || !data["Project"]["Sound"].IsDefined())
		{
			HBL2_CORE_ERROR("Project settings are incomplete or corrupted: {0}", filePath.string());
			return true;
		}

		spec.Settings.Renderer = (RendererType)data["Project"]["Renderer"]["Type"].as<int>();
		spec.Settings.EditorGraphicsAPI = (GraphicsAPI)data["Project"]["Renderer"]["Editor API"].as<int>();
		spec.Settings.RuntimeGraphicsAPI = (GraphicsAPI)data["Project"]["Renderer"]["Runtime API"].as<int>();

		spec.Settings.GravityForce2D = data["Project"]["Physics2D"]["Gravity Force"].as<float>();
		spec.Settings.EnableDebugDraw2D = data["Project"]["Physics2D"]["Enable Debug Draw"].as<bool>();
		spec.Settings.Physics2DImpl = (Physics2DEngineImpl)data["Project"]["Physics2D"]["Implementation"].as<int>();

		spec.Settings.GravityForce3D = data["Project"]["Physics3D"]["Gravity Force"].as<float>();
		spec.Settings.EnableDebugDraw3D = data["Project"]["Physics3D"]["Enable Debug Draw"].as<bool>();
		spec.Settings.ShowColliders3D = data["Project"]["Physics3D"]["Show Colliders"].as<bool>();
		spec.Settings.ShowBoundingBoxes3D = data["Project"]["Physics3D"]["Show Bounding Boxes"].as<bool>();
		spec.Settings.Physics3DImpl = (Physics3DEngineImpl)data["Project"]["Physics3D"]["Implementation"].as<int>();

		spec.Settings.SoundImpl = (SoundEngineImpl)data["Project"]["Sound"]["Implementation"].as<int>();

		spec.Settings.EditorMultipleViewports = data["Project"]["Editor"]["Multiple Viewports"].as<bool>();

		spec.Settings.MaxAppMemory = data["Project"]["Advanced"]["Max App Memory"].as<uint32_t>();
		spec.Settings.MaxUniformBufferMemory = data["Project"]["Advanced"]["Max UniformBuffer Memory"].as<uint32_t>();

		if (!data["Project"]["Advanced"]["Resource Manager"].IsDefined() || !data["Project"]["Advanced"]["Asset Manager"].IsDefined())
		{
			HBL2_CORE_ERROR("Project settings are incomplete or corrupted: {0}", filePath.string());
			return true;
		}

		spec.Settings.ResourceManagerSpec.Textures = data["Project"]["Advanced"]["Resource Manager"]["Textures Pool Size"].as<uint32_t>();
		spec.Settings.ResourceManagerSpec.Shaders = data["Project"]["Advanced"]["Resource Manager"]["Shaders Pool Size"].as<uint32_t>();
		spec.Settings.ResourceManagerSpec.Buffers = data["Project"]["Advanced"]["Resource Manager"]["Buffers Pool Size"].as<uint32_t>();
		spec.Settings.ResourceManagerSpec.BindGroups = data["Project"]["Advanced"]["Resource Manager"]["BindGroups Pool Size"].as<uint32_t>();
		spec.Settings.ResourceManagerSpec.BindGroupLayouts = data["Project"]["Advanced"]["Resource Manager"]["BindGroupLayouts Pool Size"].as<uint32_t>();
		spec.Settings.ResourceManagerSpec.FrameBuffers = data["Project"]["Advanced"]["Resource Manager"]["FrameBuffers Pool Size"].as<uint32_t>();
		spec.Settings.ResourceManagerSpec.RenderPass = data["Project"]["Advanced"]["Resource Manager"]["RenderPass Pool Size"].as<uint32_t>();
		spec.Settings.ResourceManagerSpec.RenderPassLayouts = data["Project"]["Advanced"]["Resource Manager"]["RenderPassLayouts Pool Size"].as<uint32_t>();
		spec.Settings.ResourceManagerSpec.Meshes = data["Project"]["Advanced"]["Resource Manager"]["Meshes Pool Size"].as<uint32_t>();
		spec.Settings.ResourceManagerSpec.Materials = data["Project"]["Advanced"]["Resource Manager"]["Materials Pool Size"].as<uint32_t>();
		spec.Settings.ResourceManagerSpec.Scenes = data["Project"]["Advanced"]["Resource Manager"]["Scenes Pool Size"].as<uint32_t>();
		spec.Settings.ResourceManagerSpec.Scripts = data["Project"]["Advanced"]["Resource Manager"]["Scripts Pool Size"].as<uint32_t>();
		spec.Settings.ResourceManagerSpec.Sounds = data["Project"]["Advanced"]["Resource Manager"]["Sounds Pool Size"].as<uint32_t>();
		spec.Settings.ResourceManagerSpec.Prefabs = data["Project"]["Advanced"]["Resource Manager"]["Prefabs Pool Size"].as<uint32_t>();

		spec.Settings.AssetManagerSpec.Assets = data["Project"]["Advanced"]["Asset Manager"]["Assets Pool Size"].as<uint32_t>();

		return true;
	}
}