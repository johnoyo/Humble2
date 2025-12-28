#include "OpenGLImGuiRenderer.h"

#include "Core/Context.h"
#include "Renderer/Renderer.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <filesystem>

namespace HBL2
{
	void OpenGLImGuiRenderer::Initialize()
	{
		m_GlslVersion = "#version 450 core";

		m_Window = Window::Instance;
		ImGui_ImplGlfw_InitForOpenGL(m_Window->GetHandle(), true);
		ImGui_ImplOpenGL3_Init(m_GlslVersion);
	}

	void OpenGLImGuiRenderer::BeginFrame()
	{
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void OpenGLImGuiRenderer::EndFrame()
	{
		ImGui::Render();

		{
			ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
			std::vector<ImTextureData*> pending;
			pending.reserve(pio.Textures.Size);

			for (ImTextureData* tex : pio.Textures)
			{
				if (tex->Status != ImTextureStatus_OK)
				{
					pending.push_back(tex);
				}
			}

			if (!pending.empty())
			{
				Renderer::Instance->SubmitBlocking([pending = std::move(pending)]() mutable
				{
					for (ImTextureData* tex : pending)
					{
						// NOTE: This ImGui function does not seem to trigger any errors like the vulkan one.
						ImGui_ImplOpenGL3_UpdateTexture(tex);
					}
				});
			}
		}

		{
			// Update and Render additional Platform Windows.
			ImGuiIO& io = ImGui::GetIO();

			// NOTE: If we dont call 'ImGui::UpdatePlatformWindows()' before the next 'ImGui::NewFrame()' call, we hit an assert.
			// So to prevent it, we update and render here prematurely.
			// To do this correctly we need to create the window from the main thread that has the event loop (glfwPollForEvents)
			// But, we need the main context active also to create the new window with that as shared.
			// So, we first remove the main context from active in render thread, make it current in the game thread, create the window and remove it again.
			// Finally, in the render thread we render the window and then we set the render thread to have the main context active again.
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				Renderer::Instance->SubmitBlocking([]()
				{
					glfwMakeContextCurrent(nullptr);
				});

				glfwMakeContextCurrent(Window::Instance->GetHandle());
				ImGui::UpdatePlatformWindows();
				glfwMakeContextCurrent(nullptr);

				Renderer::Instance->SubmitBlocking([]()
				{
					ImGui::RenderPlatformWindowsDefault();
					glfwMakeContextCurrent(Window::Instance->GetHandle());
				});

				Device::Instance->SetContext(ContextType::FETCH);
			}
		}

		Renderer::Instance->CollectImGuiRenderData(ImGui::GetDrawData(), ImGui::GetTime());
	}

	void OpenGLImGuiRenderer::Render(const FrameData2& frameData)
	{
		ImDrawData* data = (ImDrawData*)&frameData.ImGuiRenderData.DrawData;

		ImGui_ImplOpenGL3_NewFrame();

		int display_w, display_h;
		glfwGetFramebufferSize(m_Window->GetHandle(), &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);

		ImGui_ImplOpenGL3_RenderDrawData(data);
	}

	void OpenGLImGuiRenderer::Clean()
	{
		Renderer::Instance->ClearFrameDataBuffer();

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}