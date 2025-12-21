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
		Renderer::Instance->CollectImGuiRenderData(ImGui::GetDrawData(), ImGui::GetTime());

		// Render(ImGui::GetDrawData());
	}

	void OpenGLImGuiRenderer::Render(ImDrawData* data)
	{
		ImGui_ImplOpenGL3_NewFrame();

		int display_w, display_h;
		glfwGetFramebufferSize(m_Window->GetHandle(), &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);

		ImGui_ImplOpenGL3_RenderDrawData(data);

		ImGuiIO& io = ImGui::GetIO();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void OpenGLImGuiRenderer::Clean()
	{
		Renderer::Instance->ClearFrameDataBuffer();

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}