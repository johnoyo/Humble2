#include "EditorContext.h"
#include <Utilities\FileDialogs.h>

namespace HBL2
{
	namespace Editor
	{
		void EditorContext::OnCreate()
		{
			Mode = HBL2::Mode::Editor;
			AssetManager::Instance = new EditorAssetManager;

			// Create FrameBuffer.
			HBL2::Renderer::Instance->FrameBufferHandle = HBL2::ResourceManager::Instance->CreateFrameBuffer({
				.debugName = "editor-viewport",
				.width = 1280,
				.height = 720,
			});

			if (!OpenEmptyProject())
			{
				ActiveScene = EmptyScene;
				return;
			}

			// Create editor systems.
			EditorScene->RegisterSystem(new EditorPanelSystem);
			EditorScene->RegisterSystem(new TransformSystem);
			EditorScene->RegisterSystem(new CameraSystem);
			EditorScene->RegisterSystem(new EditorCameraSystem);

			// Editor camera set up.
			auto editorCameraEntity = EditorScene->CreateEntity("Hidden");
			EditorScene->AddComponent<HBL2::Component::Camera>(editorCameraEntity).Enabled = true;
			EditorScene->AddComponent<Component::EditorCamera>(editorCameraEntity);
			EditorScene->GetComponent<HBL2::Component::Transform>(editorCameraEntity).Translation.z = 5.f;

			// Create systems
			for (HBL2::ISystem* system : EditorScene->GetSystems())
			{
				system->OnCreate();
			}

			for (HBL2::ISystem* system : ActiveScene->GetSystems())
			{
				system->OnCreate();
			}
		}

		void EditorContext::OnUpdate(float ts)
		{
			for (HBL2::ISystem* system : EditorScene->GetSystems())
			{
				system->OnUpdate(ts);
			}

			for (HBL2::ISystem* system : ActiveScene->GetSystems())
			{
				system->OnUpdate(ts);
			}
		}

		void EditorContext::OnGuiRender(float ts)
		{
			ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

			for (HBL2::ISystem* system : EditorScene->GetSystems())
			{
				system->OnGuiRender(ts);
			}

			for (HBL2::ISystem* system : ActiveScene->GetSystems())
			{
				system->OnGuiRender(ts);
			}
		}

		bool EditorContext::OpenEmptyProject()
		{
			std::string filepath = HBL2::FileDialogs::OpenFile("Humble Project (*.hblproj)\0*.hblproj\0");

			if (HBL2::Project::Load(std::filesystem::path(filepath)) != nullptr)
			{
				RegisterAssets();

				const auto& startingScenePath = HBL2::Project::GetAssetFileSystemPath(HBL2::Project::GetActive()->GetSpecification().StartingScene);

				HBL2::Project::OpenScene(startingScenePath);

				// HBL2::Project::OpenStartingScene();

				return true;
			}

			HBL2_ERROR("Could not open specified project at path \"{0}\".", filepath);
			HBL2::Window::Instance->Close();

			return false;
		}

		void EditorContext::RegisterAssets()
		{
			for (auto& entry : std::filesystem::recursive_directory_iterator(HBL2::Project::GetAssetDirectory()))
			{
				const std::string& extension = entry.path().extension().string();
				auto relativePath = std::filesystem::relative(entry.path(), HBL2::Project::GetAssetDirectory());

				if (extension == ".png" || extension == ".jpg")
				{
					auto assetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "texture-asset",
						.filePath = relativePath,
						.type = AssetType::Texture,
					});
				}
				else if (extension == ".obj" || extension == ".gltf" || extension == ".glb" || extension == ".fbx")
				{
					auto assetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "mesh-asset",
						.filePath = relativePath,
						.type = AssetType::Mesh,
					});
				}
				else if (extension == ".hblmat")
				{
					auto assetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "material-asset",
						.filePath = relativePath,
						.type = AssetType::Material,
					});
				}
				else if (extension == ".hblshader")
				{
					auto assetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "shader-asset",
						.filePath = relativePath,
						.type = AssetType::Shader,
					});
				}
				else if (extension == ".humble")
				{
					auto assetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "scene-asset",
						.filePath = relativePath,
						.type = AssetType::Scene,
					});
				}
			}
		}
	}
}