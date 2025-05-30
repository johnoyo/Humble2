#include "OpenGLImGuiRenderer.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <filesystem>

namespace HBL2
{
	void OpenGLImGuiRenderer::Initialize()
	{
		m_Window = Window::Instance;

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		m_ImGuiContext = ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

		const auto& boldFontPath = std::filesystem::path("assets") / "fonts" / "OpenSans-Bold.ttf";
		const auto& regularFontPath = std::filesystem::path("assets") / "fonts" / "OpenSans-Regular.ttf";

		float fontSize = 18.0f;
		io.Fonts->AddFontFromFileTTF(boldFontPath.string().c_str(), fontSize);
		io.FontDefault = io.Fonts->AddFontFromFileTTF(regularFontPath.string().c_str(), fontSize);

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		SetImGuiStyle();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		m_GlslVersion = "#version 450 core";

		ImGui_ImplGlfw_InitForOpenGL(m_Window->GetHandle(), true);
		ImGui_ImplOpenGL3_Init(m_GlslVersion);
	}

	void OpenGLImGuiRenderer::BeginFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void OpenGLImGuiRenderer::EndFrame()
	{
		ImGui::Render();

		int display_w, display_h;
		glfwGetFramebufferSize(m_Window->GetHandle(), &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}