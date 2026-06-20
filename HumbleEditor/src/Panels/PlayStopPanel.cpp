#include "PlayStopPanel.h"

#include "Core\Context.h"
#include "Scene\SceneManager.h"
#include "ImGui\ImGuiRenderer.h"
#include "Systems\EditorPanelSystem.h"

namespace HBL2::Editor
{
	PlayStopPanel::PlayStopPanel(const std::string& name, EditorPanelSystem* owner)
	{
		m_Owner = owner;
		Name = name;
	}

	void PlayStopPanel::OnAttach()
	{
	}

	void PlayStopPanel::OnCreate()
	{
	}

	void PlayStopPanel::OnOpen()
	{
	}

	void PlayStopPanel::OnRender(float ts)
	{
		ImGui::Begin(Name.c_str(), &m_CloseState, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

		if (ImGui::Button("Play"))
		{
			Context::Mode = Mode::Runtime;

			if (!m_Owner->m_ActiveSceneTemp.IsValid())
			{
				m_Owner->m_ActiveSceneTemp = Context::ActiveScene;

				auto* activeScene = m_Owner->m_ActiveScene;
				auto& activeSceneDesc = activeScene->GetDescriptor();

				auto playSceneHandle = ResourceManager::Instance->CreateScene({
					.name = activeScene->GetName() + "(Clone)",
					.maxEntities = activeSceneDesc.maxEntities,
					.maxComponents = activeSceneDesc.maxComponents,
					.maxSystems = activeSceneDesc.maxSystems,
					.maxStructuralCommandsPerFramePerThread = activeSceneDesc.maxStructuralCommandsPerFramePerThread,
					.maxJobsPerSystem = activeSceneDesc.maxJobsPerSystem,
					.useStructuralCommandBuffer = activeSceneDesc.useStructuralCommandBuffer,
				});

				Scene* playScene = ResourceManager::Instance->GetScene(playSceneHandle);
				Scene::Copy(m_Owner->m_ActiveScene, playScene);
				SceneManager::Get().LoadPlaymodeScene(playSceneHandle, true);
			}

			m_IsPlaying = true;
		}

		ImGui::SameLine();

		if (ImGui::Button("Stop"))
		{
			Context::Mode = Mode::Editor;

			if (m_Owner->m_ActiveSceneTemp.IsValid())
			{
				SceneManager::Get().LoadPlaymodeScene(m_Owner->m_ActiveSceneTemp, false);
				m_Owner->m_ActiveSceneTemp = {};
			}

			m_IsPlaying = false;
		}

		ImGui::SameLine();

		if (m_IsPlaying)
		{
			ImGui::Text("Playing ... ");
		}
		else
		{
			ImGui::Text("Editing ... ");
		}

		ImGui::SameLine();

		ImGui::End();
	}

	void PlayStopPanel::OnClose()
	{
	}

	void PlayStopPanel::OnDestroy()
	{
	}
}