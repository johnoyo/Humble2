#pragma once

#include "Events.h"
#include "EventDispatcher.h"

#include <GLFW/glfw3.h>

#include <string>
#include <iostream>
#include <functional>

namespace HBL2
{
	struct HBL2_API WindowSpecification
	{
		std::string Title = "Humble App";
		float RefreshRate = 0.0f;
		float Width = 1280.f;
		float Height = 720.f;
		bool FullScreen = false;
		bool VerticalSync = false;
	};

	class HBL2_API Window
	{
	public:
		~Window() = default;
		static Window* Instance;

		void Initialize(const WindowSpecification&& spec);

		virtual void Create() = 0;
		void Terminate();
		void Close();

		void DispatchMainLoop(const std::function<void()>& mainLoop);
		void SetTitle(const std::string& title);
		double GetTime();
		GLFWwindow* GetHandle();
		GLFWwindow* GetWorkerHandle();
		glm::u32vec2 GetExtents() const { return { m_Spec.Width, m_Spec.Height }; }
		void SetExtents(uint32_t x, uint32_t y) { m_Spec.Width = x; m_Spec.Height = y; }

	protected:
		void AttachEventCallbacks();

	protected:
		GLFWwindow* m_Window = nullptr;
		GLFWwindow* m_WorkerWindow = nullptr;
		WindowSpecification m_Spec;

		static void DispatchMainEm(void* fp);
	};
}