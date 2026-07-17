#pragma once

#include "ImGui/ImGuiRenderer.h"
#include "Core/Window.h"

#include "MetalDevice.h"
#include "MetalRenderer.h"
#include "MetalResourceManager.h"
#include "MetalCommon.h"

#include <imgui.h>
#include <ImGuizmo.h>

#include <string>
#include <format>

namespace HBL2
{
    class MetalImGuiRenderer final : public ImGuiRenderer
    {
    public:
        virtual void Initialize() override;

        virtual void BeginFrame() override;
        virtual void EndFrame() override;
        virtual void Render(const FrameData& frameData) override;

        virtual void Clean() override;

    private:
        MetalDevice* m_Device = nullptr;
        MetalRenderer* m_Renderer = nullptr;
        MetalResourceManager* m_ResourceManager = nullptr;
    };
}

