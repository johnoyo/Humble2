#pragma once

#include "Base.h"

#include "Scene/Scene.h"
#include "Scene/SceneSerializer.h"
#include "Scene/SceneManager.h"

#include "Renderer/Renderer.h"
#include "Resources/ResourceManager.h"
#include "Physics/Physics.h"
#include "Physics/PhysicsEngine2D.h"
#include "Physics/PhysicsEngine3D.h"
#include "Sound/Sound.h"

#include <string>
#include <filesystem>

namespace HBL2
{
	struct ProjectSettings
	{
		RendererType Renderer = RendererType::Forward;
		GraphicsAPI EditorGraphicsAPI = GraphicsAPI::OPENGL;
		GraphicsAPI RuntimeGraphicsAPI = GraphicsAPI::VULKAN;
		
		Physics2DEngineImpl Physics2DImpl = Physics2DEngineImpl::BOX2D;
		PhysicsEngine2DSpecification PhysicsEngine2DSpec = {};
		bool EnableDebugDraw2D = false;

		Physics3DEngineImpl Physics3DImpl = Physics3DEngineImpl::JOLT;
		PhysicsEngine3DSpecification PhysicsEngine3DSpec = {};
		bool EnableDebugDraw3D = false;
		bool ShowColliders3D = false;
		bool ShowBoundingBoxes3D = false;

		SoundEngineImpl SoundImpl = SoundEngineImpl::FMOD;

		bool EditorMultipleViewports = true;
		glm::vec3 EditorCameraTranslation = { 0.f, 0.f, 0.f };
		glm::vec3 EditorCameraRotation = { 0.f, 0.f, 0.f };
		float EditorCameraNear = 0.1f;
		float EditorCameraFar = 1000.f;
		float EditorCameraFov = 30.f;
		float EditorCameraAspectRatio = 1.778f;
		float EditorCameraExposure = 1.0f;
		float EditorCameraGamma = 2.2f;
		float EditorCameraZoomLevel = 300.f;
		float EditorCameraMovementSpeed = 24.0f;
		float EditorCameraMouseSensitivity = 0.1f;
		float EditorCameraPanSpeed = 0.25f;
		float EditorCameraZoomSpeed = 1.0f;
		float EditorCameraScrollZoomSpeed = 10.0f;

		uint32_t MaxAppMemory = 500; // In MB
		uint32_t MaxMainThreadFrameArenaMemory = 32; // In MB
		uint32_t MaxRenderThreadFrameArenaMemory = 8; // In MB
		uint32_t MaxWorkerMemory = 2; // In MB
		uint32_t MaxUniformBufferMemory = 32; // In MB
		ResourceManagerSpecification ResourceManagerSpec = {};
		AssetManagerSpecification AssetManagerSpec = {};
	};

	struct HBL2_API ProjectSpecification
	{
		std::string Name = "Untitled";

		std::filesystem::path StartingScene;
		std::filesystem::path AssetDirectory;
		std::filesystem::path ScriptDirectory;

		ProjectSettings Settings;
	};

	class HBL2_API Project
	{
	public:
		Project() = default;

		ProjectSpecification& GetSpecification() { return m_Spec; }

		static Project* GetActive() { return s_ActiveProject; }

		static const std::filesystem::path& GetProjectDirectory()
		{
			HBL2_CORE_ASSERT(s_ActiveProject, "Active project is null.");
			return s_ActiveProject->m_ProjectDirectory;
		}

		static std::filesystem::path GetAssetDirectory()
		{
			HBL2_CORE_ASSERT(s_ActiveProject, "Active project is null.");
			return GetProjectDirectory() / s_ActiveProject->m_Spec.AssetDirectory;
		}

		static std::filesystem::path GetAssetFileSystemPath(const std::filesystem::path& path)
		{
			HBL2_CORE_ASSERT(s_ActiveProject, "Active project is null.");
			return GetAssetDirectory() / path;
		}

		static const std::string& GetName()
		{
			return s_ActiveProject->m_Spec.Name;
		}

		static Project* Create(const std::string& name = "");
		static Project* Load(const std::filesystem::path& path);
		static void Save(const std::filesystem::path& path = "");

		static void OpenStartingScene(bool runtime = false);
		static void ApplySettings();

	private:
		ProjectSpecification m_Spec;
		std::filesystem::path m_ProjectDirectory;
		std::filesystem::path m_ProjectFilePath;

		static Project* s_ActiveProject;
	};
}
