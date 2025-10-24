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
		out << YAML::EndMap;

		out << YAML::Key << "Physics2D" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Gravity Force" << YAML::Value << spec.Settings.GravityForce2D;
		out << YAML::Key << "Show Colliders" << YAML::Value << spec.Settings.ShowPhysicsColliders2D;
		out << YAML::EndMap;

		out << YAML::Key << "Physics3D" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Gravity Force" << YAML::Value << spec.Settings.GravityForce2D;
		out << YAML::Key << "Show Colliders" << YAML::Value << spec.Settings.ShowPhysicsColliders3D;
		out << YAML::Key << "Show Only Bounding Boxes" << YAML::Value << spec.Settings.ShowOnlyBoundingBoxes3D;
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

		if (!data["Project"]["Renderer"].IsDefined() ||	!data["Project"]["Physics2D"].IsDefined() || !data["Project"]["Physics3D"].IsDefined())
		{
			HBL2_CORE_ERROR("Project settings are incomplete or corrupted: {0}", filePath.string());
			return true;
		}

		spec.Settings.Renderer = (RendererType)data["Project"]["Renderer"]["Type"].as<int>();
		spec.Settings.GravityForce2D = data["Project"]["Physics2D"]["Gravity Force"].as<float>();
		spec.Settings.ShowPhysicsColliders2D = data["Project"]["Physics2D"]["Show Colliders"].as<bool>();
		spec.Settings.GravityForce3D = data["Project"]["Physics3D"]["Gravity Force"].as<float>();
		spec.Settings.ShowPhysicsColliders3D = data["Project"]["Physics3D"]["Show Colliders"].as<bool>();
		spec.Settings.ShowOnlyBoundingBoxes3D = data["Project"]["Physics3D"]["Show Only Bounding Boxes"].as<bool>();

		return true;
	}
}