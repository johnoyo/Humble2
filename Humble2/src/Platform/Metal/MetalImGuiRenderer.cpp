#include "MetalImGuiRenderer.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_metal4.h>

#include "Core/Input.h"

namespace HBL2
{
    void MetalImGuiRenderer::Initialize()
    {
        m_Device = (MetalDevice*)Device::Instance;
        m_Renderer = (MetalRenderer*)Renderer::Instance;
        m_ResourceManager = (MetalResourceManager*)ResourceManager::Instance;

        ImGui_ImplGlfw_InitForOther(Window::Instance->GetHandle(), true);
        ImGui_ImplMetal4_Init(m_Device->Get(), m_Renderer->GetCommandQueue(), FRAME_OVERLAP);
    }

    void MetalImGuiRenderer::BeginFrame()
    {
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void MetalImGuiRenderer::EndFrame()
    {
        ImGui::Render();
        m_Renderer->CollectImGuiRenderData(ImGui::GetDrawData(), ImGui::GetTime());
    }

    void MetalImGuiRenderer::Render(const FrameData& frameData)
    {
        ImDrawData* data = (ImDrawData*)&frameData.ImGuiRenderData.DrawData;

        // TODO: Construct a MTL4::RenderPassDescriptor from the m_Renderer->GetImGuiRenderPass().
        ImGui_ImplMetal4_NewFrame(nullptr, m_Renderer->GetFrameIndex());

        CommandBuffer* commandBuffer = m_Renderer->BeginCommandRecording(CommandBufferType::UI);
        RenderPassRenderer* renderPassRenderer = commandBuffer->BeginRenderPass(m_Renderer->GetImGuiRenderPass());

        ImGui_ImplMetal4_RenderDrawData(data, m_Renderer->GetCurrentFrame().ImGuiCommandBuffer, ((MetalRenderPassRenderer*)renderPassRenderer)->Encoder);

        commandBuffer->EndRenderPass(*renderPassRenderer);
        commandBuffer->EndCommandRecording();
        commandBuffer->Submit();
    }

    void MetalImGuiRenderer::Clean()
    {
        m_Renderer->ClearFrameDataBuffer();

        ImGui_ImplMetal4_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}

