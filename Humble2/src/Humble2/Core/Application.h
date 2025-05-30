#pragma once

#include "Platform\OpenGL\OpenGLWindow.h"
#include "Platform\Vulkan\VulkanWindow.h"

#include "Base.h"
#include "Input.h"
#include "Time.h"
#include "Allocators.h"
#include "Context.h"
#include "EventDispatcher.h"

#include "ImGui\ImGuiRenderer.h"
#include "Renderer\Device.h"
#include "Renderer\Renderer.h"
#include "Resources\ResourceManager.h"

#include "Platform\OpenGL\OpenGLResourceManager.h"
#include "Platform\OpenGL\OpenGLImGuiRenderer.h"
#include "Platform\OpenGL\OpenGLRenderer.h"
#include "Platform\OpenGL\OpenGLDevice.h"

#include "Platform\Vulkan\VulkanImGuiRenderer.h"
#include "Platform\Vulkan\VulkanResourceManager.h"
#include "Platform\Vulkan\VulkanRenderer.h"
#include "Platform\Vulkan\VulkanDevice.h"

#include "Scene\SceneManager.h"

#include "Utilities\JobSystem.h"
#include "Utilities\Random.h"
#include "Utilities\MeshUtilities.h"
#include "Utilities\ShaderUtilities.h"
#include "Utilities\NativeScriptUtilities.h"
#include "Utilities\UnityBuild.h"

#include "Utilities/Allocators/BumpAllocator.h"
#include "Utilities/Allocators/BinAllocator.h"

#include <string>
#include <sstream>
#include <iostream>

namespace HBL2
{
	enum class HBL2_API Platform
	{
		Windows,
		Web,
		None
	};

	struct HBL2_API ApplicationSpec
	{
		std::string Name = "Humble2 Application";
		std::string CommandLineArgs = "";
		GraphicsAPI GraphicsAPI = GraphicsAPI::OPENGL;
		float Width = 1600.f;
		float Height = 900.f;
		bool VerticalSync = true;
		bool Fullscreen = false;

		Context* Context = nullptr;
	};

	class HBL2_API Application
	{
	public:
		Application(ApplicationSpec& specification);
		~Application();

		void Start();

		static Application& Get()
		{
			return *s_Instance;
		}

		const ApplicationSpec& GetSpec() const { return m_Specification; }

	private:
		static Application* s_Instance;

		ApplicationSpec m_Specification;

		float m_LastTime = 0.0f;
		float m_Timer = m_LastTime;
		int m_Frames = 0, m_Updates = 0;

		int m_FixedUpdates = 0;

		void BeginFrame();
		void EndFrame();
		void Shutdown();
	};
}