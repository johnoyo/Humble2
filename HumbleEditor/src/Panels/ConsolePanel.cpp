#include "Systems\EditorPanelSystem.h"

namespace HBL2
{
	namespace Editor
	{
        struct ConsoleViewState
        {
            bool ShowTrace = true;
            bool ShowInfo = true;
            bool ShowWarn = true;
            bool ShowError = true;
            bool ShowFatal = true;

            bool AutoScroll = true;
            bool Pause = false;

            ImGuiTextFilter Filter;
        };

        static bool PassesTypeFilter(const ConsoleViewState& s, MessageInfo::MessageType t)
        {
            switch (t)
            {
            case MessageInfo::MessageType::ETRACE: return s.ShowTrace;
            case MessageInfo::MessageType::EINFO:  return s.ShowInfo;
            case MessageInfo::MessageType::EWARN:  return s.ShowWarn;
            case MessageInfo::MessageType::EERROR: return s.ShowError;
            case MessageInfo::MessageType::EFATAL: return s.ShowFatal;
            default: return true;
            }
        }

        static ImVec4 ColorForType(MessageInfo::MessageType t)
        {
            switch (t)
            {
            case MessageInfo::MessageType::ETRACE: return { 1.f, 1.f, 1.f, 1.f };
            case MessageInfo::MessageType::EINFO:  return { 0.f, 1.f, 0.f, 1.f };
            case MessageInfo::MessageType::EWARN:  return { 1.f, 1.f, 0.f, 1.f };
            case MessageInfo::MessageType::EERROR: return { 1.f, 0.1f, 0.f, 1.f };
            case MessageInfo::MessageType::EFATAL: return { 1.f, 0.f, 0.f, 1.f };
            default:                               return { 1.f, 1.f, 1.f, 1.f };
            }
        }

        static bool PassesSearch(const ConsoleViewState& s, const MessageInfo& msg)
        {
            // No filter text then accept.
            if (s.Filter.IsActive() == false)
            {
                return true;
            }

            // ImGuiTextFilter::PassFilter wants a null-terminated string.
            char tmp[1024];

            const int tagLen = (int)msg.Message.size();
            const int msgLen = (int)msg.Message.size();

            // Format: "[TAG] message", clamp to avoid overflow
            int written = 0;
            written += snprintf(tmp + written, sizeof(tmp) - written, "[%.*s] ", tagLen, msg.Tag);
            if (written < (int)sizeof(tmp))
            {
                written += snprintf(tmp + written, sizeof(tmp) - written, "%.*s", msgLen, msg.Message);
            }

            tmp[sizeof(tmp) - 1] = '\0';
            return s.Filter.PassFilter(tmp);
        }

        static void DrawSplitterVertical(float& topHeight, float& bottomHeight, float minTop, float minBottom)
        {
            // Draw an invisible splitter bar between top and bottom.
            const float splitterThickness = 6.0f;

            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImVec2 cursor = ImGui::GetCursorPos();

            // Place splitter at current cursor. We'll position it after the top child.
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

            ImGui::Button("##splitter", ImVec2(-1.0f, splitterThickness));

            ImGui::PopStyleColor(3);

            if (ImGui::IsItemActive())
            {
                float delta = ImGui::GetIO().MouseDelta.y;
                topHeight += delta;
                bottomHeight -= delta;

                if (topHeight < minTop)
                {
                    bottomHeight -= (minTop - topHeight);
                    topHeight = minTop;
                }

                if (bottomHeight < minBottom)
                {
                    topHeight -= (minBottom - bottomHeight);
                    bottomHeight = minBottom;
                }
            }
        }

        void EditorPanelSystem::DrawConsolePanel(float ts)
        {
            static ConsoleViewState view;

            // Persistent UI state
            static uint64_t selectedSeq = UINT64_MAX;
            static bool detailsOpen = true;
            static float detailsHeight = 160.0f; // only meaningful when open

            const float minListHeight = 80.0f;
            const float minDetailsHeight = 80.0f;
            const float splitterThickness = 6.0f;

            // Toolbar
            {
                ImGui::Checkbox("T", &view.ShowTrace); ImGui::SameLine();
                ImGui::Checkbox("I", &view.ShowInfo);  ImGui::SameLine();
                ImGui::Checkbox("W", &view.ShowWarn);  ImGui::SameLine();
                ImGui::Checkbox("E", &view.ShowError); ImGui::SameLine();
                ImGui::Checkbox("F", &view.ShowFatal); ImGui::SameLine();

                if (ImGui::Button("Clear"))
                {
                    Console::Instance->ClearLog();
                    selectedSeq = UINT64_MAX;
                }

                ImGui::SameLine();
                ImGui::Checkbox("Auto-scroll", &view.AutoScroll);

                ImGui::SameLine();
                ImGui::Checkbox("Pause", &view.Pause);

                ImGui::SameLine();
                view.Filter.Draw("Search", 240.0f);

                ImGui::Separator();
            }

            // Parent child so we can split inside the remaining space
            ImGui::BeginChild("ConsoleContent", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            // Snapshot (pause support)
            static ConsoleSnapshot pausedSnap{};
            const ConsoleSnapshot snap = view.Pause ? pausedSnap : Console::Instance->GetSnapshot();
            if (!view.Pause)
            {
                pausedSnap = snap;
            }

            const uint64_t startSeq = (snap.Count == 0) ? 0 : (snap.EndSeq - snap.Count + 1);
            const float lineH = ImGui::GetTextLineHeightWithSpacing();
            const float collapsedHeaderH = ImGui::GetFrameHeight(); // details header height when collapsed

            // Build visible seq list
            static std::vector<uint64_t> visibleSeqs;
            visibleSeqs.clear();
            visibleSeqs.reserve(snap.Count);

            for (uint32_t i = 0; i < snap.Count; ++i)
            {
                const uint64_t seq = startSeq + i;

                MessageInfo msg;
                if (!Console::Instance->TryReadMessageAtSequence(seq, msg))
                {
                    continue;
                }

                if (!PassesTypeFilter(view, msg.Type))
                {
                    continue;
                }

                if (!PassesSearch(view, msg))
                {
                    continue;
                }

                visibleSeqs.push_back(seq);
            }

            // Is selection currently visible (filtered-in)?
            bool selectedVisible = false;
            if (selectedSeq != UINT64_MAX)
            {
                for (uint64_t s : visibleSeqs)
                {
                    if (s == selectedSeq)
                    {
                        selectedVisible = true;
                        break;
                    }
                }
            }

            // Compute top/bottom heights
            const float availH = ImGui::GetContentRegionAvail().y;

            float effectiveDetailsH = detailsOpen ? detailsHeight : collapsedHeaderH;
            effectiveDetailsH = std::clamp(effectiveDetailsH, collapsedHeaderH, std::max(collapsedHeaderH, availH - splitterThickness - minListHeight));

            float listHeight = availH - splitterThickness - effectiveDetailsH;
            if (listHeight < minListHeight)
            {
                listHeight = minListHeight;
                effectiveDetailsH = std::max(collapsedHeaderH, availH - splitterThickness - listHeight);
            }

            if (detailsOpen)
            {
                detailsHeight = std::max(minDetailsHeight, effectiveDetailsH);
            }

            // Top scrolling list for incoming logs.
            ImGui::BeginChild("ConsoleScrollRegion", ImVec2(0, listHeight), false, ImGuiWindowFlags_HorizontalScrollbar);

            const float beforeScrollY = ImGui::GetScrollY();
            const float beforeMaxY = ImGui::GetScrollMaxY();
            const bool wasAtBottom = (beforeMaxY - beforeScrollY) < 5.0f;

            ImGuiListClipper clipper;
            clipper.Begin((int)visibleSeqs.size(), lineH);

            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                {
                    const uint64_t seq = visibleSeqs[i];

                    MessageInfo msg;
                    if (!Console::Instance->TryReadMessageAtSequence(seq, msg))
                    {
                        continue;
                    }

                    const bool isSelected = (seq == selectedSeq);

                    ImGui::PushID((int)(seq ^ (seq >> 32)));

                    // Build row label (single line)
                    char label[1024];
                    int written = snprintf(label, sizeof(label), "[%.*s] %.*s", msg.Message.size(), msg.Tag, msg.Message.size(), msg.Message);
                    (void)written;
                    label[sizeof(label) - 1] = '\0';

                    // Color just the text, keep selection highlight default
                    ImGui::PushStyleColor(ImGuiCol_Text, ColorForType(msg.Type));
                    if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_SpanAvailWidth))
                    {
                        selectedSeq = seq;
                    }
                    ImGui::PopStyleColor();

                    ImGui::PopID();
                }
            }

            // Auto-scroll
            if (view.AutoScroll)
            {
                if (wasAtBottom && !view.Pause)
                {
                    ImGui::SetScrollHereY(1.0f);
                }
            }

            ImGui::EndChild();

            // Splitter (only when open).
            if (detailsOpen)
            {
                float topH = listHeight;
                float botH = detailsHeight;

                // Inline splitter (uses the same behavior as the helper shown before)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

                ImGui::Button("##splitter", ImVec2(-1.0f, splitterThickness));

                ImGui::PopStyleColor(3);

                if (ImGui::IsItemActive())
                {
                    const float delta = ImGui::GetIO().MouseDelta.y;

                    topH += delta;
                    botH -= delta;

                    if (topH < minListHeight) { botH -= (minListHeight - topH); topH = minListHeight; }
                    if (botH < minDetailsHeight) { topH -= (minDetailsHeight - botH); botH = minDetailsHeight; }

                    detailsHeight = botH;
                }
            }
            else
            {
                // When collapsed, just a subtle separator line between list and header
                ImGui::Separator();
            }

            // Bottom collapsible details.
            {
                ImGuiTreeNodeFlags headerFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;

                // CollapsingHeader returns open/closed; it also toggles when clicked.
                const bool nowOpen = ImGui::CollapsingHeader("Details", headerFlags);
                detailsOpen = nowOpen;

                if (detailsOpen)
                {
                    // remaining space we reserved for details includes the header height,
                    // so child height is (detailsHeight - headerHeight)
                    float contentH = detailsHeight - ImGui::GetFrameHeight();
                    if (contentH < 1.0f) contentH = 1.0f;

                    ImGui::BeginChild("ConsoleDetails", ImVec2(0, contentH), true);

                    if (selectedSeq == UINT64_MAX)
                    {
                        ImGui::TextUnformatted("Click a log entry to see details.");
                    }
                    else
                    {
                        MessageInfo sel{};
                        const bool ok = Console::Instance->TryReadMessageAtSequence(selectedSeq, sel);

                        if (!ok)
                        {
                            ImGui::Text("Selected log (seq=%llu) is no longer available.", (unsigned long long)selectedSeq);
                        }
                        else
                        {
                            if (!selectedVisible)
                            {
                                ImGui::TextUnformatted("(Selected log is currently hidden by filters)");
                            }

                            ImGui::PushStyleColor(ImGuiCol_Text, ColorForType(sel.Type));
                            ImGui::Text("[%.*s] %.*s", sel.Tag.size(), sel.Tag, sel.Message.size(), sel.Message);
                            ImGui::PopStyleColor();

                            ImGui::Spacing();

                            const auto fileView = sel.File.view();
                            const auto funcView = sel.Function.view();

                            ImGui::Text("File: %.*s", (int)fileView.size(), fileView.data());
                            ImGui::Text("Line: %u", sel.Line);
                            ImGui::Text("Function: %.*s", (int)funcView.size(), funcView.data());
                        }
                    }

                    ImGui::EndChild();
                }
            }

            ImGui::EndChild(); // ConsoleContent
        }
	}
}