#include "GameViewportPanel.h"

#include "Systems/EditorPanelSystem.h"

namespace HBL2::Editor
{
	GameViewportPanel::GameViewportPanel(const std::string& name, EditorPanelSystem* owner)
	{
		m_Owner = owner;
		Name = name;
	}

	void GameViewportPanel::OnAttach()
	{
	}

	void GameViewportPanel::OnCreate()
	{
	}

	void GameViewportPanel::OnOpen()
	{
	}

	void GameViewportPanel::OnRender(float ts)
	{
		Scene* ctx = m_Owner->m_Context;
		Scene* activeScene = m_Owner->m_ActiveScene;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.f, 0.f });

		ImGui::Begin(Name.c_str(), &m_CloseState);

		Context::ViewportSize = { ImGui::GetWindowWidth(), ImGui::GetWindowHeight() };
		Context::ViewportPosition = { ImGui::GetWindowPos().x, ImGui::GetWindowPos().y };

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		if (m_Owner->m_GameViewportSize != *(glm::vec2*)&viewportPanelSize)
		{
			m_Owner->m_GameViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
			EventDispatcher::Get().Post(GameViewportSizeEvent(m_Owner->m_GameViewportSize.x, m_Owner->m_GameViewportSize.y));

			if (activeScene != nullptr)
			{
				activeScene->Filter<HBL2::Component::Camera>()
					.ForEach([&](HBL2::Component::Camera& camera)
					{
						if (camera.Enabled)
						{
							if (m_Owner->m_GameViewportSize.y != 0.0f)
							{
								camera.AspectRatio = m_Owner->m_GameViewportSize.x / m_Owner->m_GameViewportSize.y;
							}
						}
					});
			}
		}

		ImTextureID viewportTexture = (ImTextureID)HBL2::Renderer::Instance->GetGameViewAttachment();

		if (viewportTexture != 0)
		{
			ImGui::Image(viewportTexture, ImVec2{ m_Owner->m_GameViewportSize.x, m_Owner->m_GameViewportSize.y });

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Scene"))
				{
					uint32_t sceneAssetHandlePacked = *((uint32_t*)payload->Data);
					Handle<Asset> sceneAssetHandle = Handle<Asset>::UnPack(sceneAssetHandlePacked);
					Asset* sceneAsset = AssetManager::Instance->GetAssetMetadata(sceneAssetHandle);

					if (sceneAsset == nullptr)
					{
						HBL2_WARN("Could not load scene - invalid asset handle.");
						ImGui::EndDragDropTarget();
						return;
					}

					auto path = HBL2::Project::GetAssetFileSystemPath(sceneAsset->FilePath);

					if (path.extension().string() != ".humble")
					{
						HBL2_WARN("Could not load {0} - not a scene file", path.filename().string());
						ImGui::EndDragDropTarget();
						return;
					}

					if (Context::Mode == Mode::Runtime)
					{
						HBL2_WARN("Could not load {0} - exit play mode and then change scenes.", path.filename().string());
						ImGui::EndDragDropTarget();
						return;
					}

					HBL2::SceneManager::Get().LoadScene(sceneAssetHandle, false);

					m_Owner->m_EditorScenePath = path;
				}

				ImGui::EndDragDropTarget();
			}
		}

		ImGui::End();

		ImGui::PopStyleVar();
	}

	void GameViewportPanel::OnClose()
	{
	}

	void GameViewportPanel::OnDestroy()
	{
	}
}
