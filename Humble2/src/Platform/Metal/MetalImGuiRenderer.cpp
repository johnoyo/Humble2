#include "MetalImGuiRenderer.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_metal.h>

#include "Core/Input.h"

namespace HBL2
{
    void MetalImGuiRenderer::Initialize()
    {
        m_Device = (MetalDevice*)Device::Instance;
        m_Renderer = (MetalRenderer*)Renderer::Instance;
        m_ResourceManager = (MetalResourceManager*)ResourceManager::Instance;

        ImGui_ImplGlfw_InitForOther(Window::Instance->GetHandle(), true);
        ImGui_ImplMetal_Init((MTL::Device*)m_Device->Get());
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

        ImGui_ImplMetal_NewFrame(nullptr);

        CommandBuffer* commandBuffer = m_Renderer->BeginCommandRecording(CommandBufferType::UI);
        RenderPassRenderer* renderPassRenderer = commandBuffer->BeginRenderPass(m_Renderer->GetImGuiRenderPass());

        {
            // Lock the queue here since the imgui function may mess with it.
            //std::lock_guard<std::mutex> lock(m_Renderer->GetGraphicsQueueMutex());
            //ImGui_ImplVulkan_RenderDrawData(data, m_Renderer->GetCurrentFrame().ImGuiCommandBuffer);
            
            ImGui_ImplMetal_RenderDrawData(data, nullptr, nullptr);
        }

        commandBuffer->EndRenderPass(*renderPassRenderer);
        commandBuffer->EndCommandRecording();
        commandBuffer->Submit();
    }

    void MetalImGuiRenderer::Clean()
    {
        m_Renderer->ClearFrameDataBuffer();

        ImGui_ImplMetal_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}

