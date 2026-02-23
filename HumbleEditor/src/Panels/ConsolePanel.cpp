#include "Systems\EditorPanelSystem.h"

namespace HBL2
{
	namespace Editor
	{
		void EditorPanelSystem::DrawConsolePanel(float ts)
		{
			auto snap = Console::Instance->GetSnapshot();
			const uint64_t startSeq = (snap.Count == 0) ? 0 : (snap.EndSeq - snap.Count + 1);

			for (uint32_t i = 0; i < snap.Count; ++i)
			{
				const uint64_t seq = startSeq + i;

				MessageInfo msg;
				if (Console::Instance->TryReadMessageAtSequence(seq, msg))
				{
					switch (msg.Type)
					{
					case MessageInfo::MessageType::ETRACE:
						ImGui::PushStyleColor(ImGuiCol_Text, { 1.f, 1.f, 1.f, 1.f });
						break;
					case MessageInfo::MessageType::EINFO:
						ImGui::PushStyleColor(ImGuiCol_Text, { 0.f, 1.f, 0.f, 1.f });
						break;
					case MessageInfo::MessageType::EWARN:
						ImGui::PushStyleColor(ImGuiCol_Text, { 1.f, 1.f, 0.f, 1.f });
						break;
					case MessageInfo::MessageType::EERROR:
						ImGui::PushStyleColor(ImGuiCol_Text, { 1.f, 0.1f, 0.f, 1.f });
						break;
					case MessageInfo::MessageType::ECRITICAL:
						ImGui::PushStyleColor(ImGuiCol_Text, { 1.f, 0.f, 0.f, 1.f });
						break;
					}

					ImGui::Text("[%.*s] %.*s", msg.TagLen, msg.Tag, msg.MsgLen, msg.Message);

					ImGui::PopStyleColor();
				}
			}
		}
	}
}