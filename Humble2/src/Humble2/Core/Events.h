#pragma once

#include "EventDispatcher.h"
#include "Resources\Handle.h"

namespace HBL2
{
	class Scene;
	class ISystem;
	enum class SystemState;

	class HBL2_API WindowCloseEvent final : public Event
	{
	public:
		WindowCloseEvent()
		{
		}

		virtual std::string GetDescription() const override
		{
			return "WindowCloseEvent";
		}
	};

	class HBL2_API WindowSizeEvent final : public Event
	{
	public:
		WindowSizeEvent(int width, int height) : Width(width), Height(height)
		{
		}

		virtual std::string GetDescription() const override
		{
			return "WindowSizeEvent";
		}

		int Width = 0;
		int Height = 0;
	};

	class HBL2_API FramebufferSizeEvent final : public Event
	{
	public:
		FramebufferSizeEvent(int width, int height) : Width(width), Height(height)
		{
		}

		virtual std::string GetDescription() const override
		{
			return "FramebufferSizeEvent";
		}

		int Width = 0;
		int Height = 0;
	};

	class HBL2_API ViewportSizeEvent final : public Event
	{
	public:
		ViewportSizeEvent(int width, int height) : Width(width), Height(height)
		{
		}

		virtual std::string GetDescription() const override
		{
			return "ViewportSizeEvent";
		}

		int Width = 0;
		int Height = 0;
	};

	class HBL2_API WindowFocusEvent final : public Event
	{
	public:
		WindowFocusEvent(int focused) : Focused(focused)
		{
		}

		virtual std::string GetDescription() const override
		{
			return "WindowFocusEvent";
		}

		int Focused = 0;
	};

	class HBL2_API WindowRefreshEvent final : public Event
	{
	public:
		WindowRefreshEvent()
		{
		}

		virtual std::string GetDescription() const override
		{
			return "WindowRefreshEvent";
		}
	};

	class HBL2_API MouseButtonEvent final : public Event
	{
	public:
		MouseButtonEvent(int button, int action, int mods) : Button(button), Action(action), Mods(mods)
		{
		}

		virtual std::string GetDescription() const override
		{
			return "MouseButtonEvent";
		}

		int Button;
		int Action;
		int Mods;
	};

	class HBL2_API ScrollEvent final : public Event
	{
	public:
		ScrollEvent(double xoffset, double yoffset) : XOffset(xoffset), YOffset(yoffset)
		{
		}

		virtual std::string GetDescription() const override
		{
			return "ScrollEvent";
		}

		double XOffset;
		double YOffset;
	};

	class HBL2_API CursorPositionEvent final : public Event
	{
	public:
		CursorPositionEvent(double xpos, double ypos) : XPosition(xpos), YPosition(ypos)
		{
		}

		virtual std::string GetDescription() const override
		{
			return "CursorPositionEvent";
		}

		double XPosition;
		double YPosition;
	};

	class HBL2_API CursorEnterEvent final : public Event
	{
	public:
		CursorEnterEvent(int entered) : Entered(entered)
		{
		}

		virtual std::string GetDescription() const override
		{
			return "CursorEnterEvent";
		}

		int Entered;
	};

	class HBL2_API KeyEvent final : public Event
	{
	public:
		KeyEvent(int key, int scancode, int action, int mods) : Key(key), Scancode(scancode), Action(action), Mods(mods)
		{
		}

		virtual std::string GetDescription() const override
		{
			return "KeyEvent";
		}

		int Key;
		int Scancode;
		int Action;
		int Mods;
	};

	class HBL2_API SceneChangeEvent final : public Event
	{
	public:
		SceneChangeEvent(Handle<Scene> oldScene, Handle<Scene> newScene) : OldScene(oldScene), NewScene(newScene)
		{
		}

		virtual std::string GetDescription() const override
		{
			return "SceneChangeEvent";
		}

		Handle<Scene> OldScene;
		Handle<Scene> NewScene;
	};

	class HBL2_API SystemStateChangeEvent final : public Event
	{
	public:
		SystemStateChangeEvent(ISystem* system, SystemState oldState, SystemState newState) : System(system), OldState(oldState), NewState(newState)
		{
		}

		virtual std::string GetDescription() const override
		{
			return "SystemStateChangeEvent";
		}

		ISystem* System;
		SystemState OldState;
		SystemState NewState;
	};
}