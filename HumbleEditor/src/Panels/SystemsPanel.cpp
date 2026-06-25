#include "SystemsPanel.h"

#include "Script/BuildEngine.h"
#include "ImGui/ImGuiRenderer.h"
#include "Systems/EditorPanelSystem.h"

namespace HBL2::Editor
{
	SystemsPanel::SystemsPanel(const std::string& name, EditorPanelSystem* owner)
	{
		m_Owner = owner;
		Name = name;
	}

	void SystemsPanel::OnAttach()
	{
	}

	void SystemsPanel::OnCreate()
	{
	}

	void SystemsPanel::OnOpen()
	{
	}
	
	void SystemsPanel::OnRender(float ts)
	{
		ImGui::Begin(Name.c_str(), &m_CloseState);

		if (m_Owner->m_ActiveScene != nullptr)
		{
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Script"))
				{
					uint32_t scriptAssetHandlePacked = *((uint32_t*)payload->Data);
					Handle<Asset> scriptAssetHandle = Handle<Asset>::UnPack(scriptAssetHandlePacked);
					Asset* scriptAsset = AssetManager::Instance->GetAssetMetadata(scriptAssetHandle);

					if (scriptAsset == nullptr)
					{
						HBL2_WARN("Could not load script - invalid asset handle.");
					}
					else
					{
						Handle<Script> scriptHandle = AssetManager::Instance->GetAsset<Script>(scriptAssetHandle);

						if (scriptHandle.IsValid())
						{
							Script* script = ResourceManager::Instance->GetScript(scriptHandle);

							if (script->Type == ScriptType::SYSTEM)
							{
								BuildEngine::Instance->RegisterSystem(script->Name, m_Owner->m_ActiveScene);
							}
							else
							{
								HBL2_WARN("Could not load script - not a system script.");
							}
						}
						else
						{
							HBL2_WARN("Could not load script - invalid script handle.");
						}
					}
				}

				ImGui::EndDragDropTarget();
			}

			ImGui::TextWrapped(m_Owner->m_ActiveScene->GetName().c_str());
			ImGui::NewLine();
			ImGui::Separator();

			ISystem* systemToBeDeregistered = nullptr;

			for (ISystem* system : m_Owner->m_ActiveScene->GetSystems())
			{
				ImGui::TextWrapped(system->Name.c_str());

				const std::string& name = "Deregister System " + system->Name;

				if (ImGui::BeginPopupContextItem(name.c_str()))
				{
					if (ImGui::MenuItem("Deregister"))
					{
						systemToBeDeregistered = system;
					}
					ImGui::EndPopup();
				}

				ImGui::SameLine(ImGui::GetWindowWidth() - 80.0f);
				switch (system->GetState())
				{
				case SystemState::Play:
					ImGui::Button(("Pause##" + system->Name).c_str(), ImVec2(60.f, 30.f));
					if (ImGui::IsItemClicked())
					{
						system->SetState(SystemState::Pause);
					}
					break;
				case SystemState::Pause:
					ImGui::Button(("Resume##" + system->Name).c_str(), ImVec2(60.f, 30.f));
					if (ImGui::IsItemClicked())
					{
						system->SetState(SystemState::Play);
					}
					break;
				case SystemState::Idle:
					ImGui::Button(("Idling##" + system->Name).c_str(), ImVec2(60.f, 30.f));
					break;
				}
				ImGui::Separator();
			}

			if (systemToBeDeregistered != nullptr)
			{
				m_Owner->m_ActiveScene->DeregisterSystem(systemToBeDeregistered);
			}
		}

		ImGui::End();
	}
	
	void SystemsPanel::OnClose()
	{
	}
	
	void SystemsPanel::OnDestroy()
	{
	}
}
