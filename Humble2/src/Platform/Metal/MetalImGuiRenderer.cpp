#include "MetalImGuiRenderer.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_metal4.h>

#include "Core/Input.h"

namespace HBL2
{
    static MTL4::RenderPassDescriptor* ExtractRenderPassDescriptor(Handle<RenderPass> rpH)
    {
        auto* rm = (MetalResourceManager*)ResourceManager::Instance;
        
        MetalRenderPass* rp = rm->GetRenderPass(rpH);
        if (rp != nullptr)
        {
            return rp->PassDesc;
        }
        
        return nullptr;
    }

    void MetalImGuiRenderer::Initialize()
    {
        m_Device = (MetalDevice*)Device::Instance;
        m_Renderer = (MetalRenderer*)Renderer::Instance;
        m_ResourceManager = (MetalResourceManager*)ResourceManager::Instance;

        ImGui_ImplGlfw_InitForOther(Window::Instance->GetHandle(), true);
        ImGui_ImplMetal4_Init(m_Device->Get(), m_Renderer->GetCommandQueue(), FRAME_OVERLAP);
        
        // Set viewport texture.
        MetalTexture* viewportTexture = m_ResourceManager->GetTexture(m_Renderer->MainColorTexture);
        m_Renderer->SetViewportAttachment((void*)viewportTexture->Texture);
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

        MTL4::RenderPassDescriptor* renderPassDescriptor = ExtractRenderPassDescriptor(m_Renderer->GetImGuiRenderPass());
        ImGui_ImplMetal4_NewFrame(renderPassDescriptor, m_Renderer->GetFrameIndex());

        MetalCommandBuffer* commandBuffer = (MetalCommandBuffer*)m_Renderer->BeginCommandRecording(CommandBufferType::UI);
        RenderPassRenderer* rpRenderer = commandBuffer->BeginRenderPass(m_Renderer->GetImGuiRenderPass());

        ImGui_ImplMetal4_RenderDrawData(data, commandBuffer->CommandBuffer, ((MetalRenderPassRenderer*)rpRenderer)->Encoder);

        commandBuffer->EndRenderPass(*rpRenderer);
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

