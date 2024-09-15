#pragma once

#include "EventDispatcher.h"

#include "Scene\Scene.h"
#include "Resources\Handle.h"

namespace HBL2
{
	class WindowCloseEvent final : public Event
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

	class WindowSizeEvent final : public Event
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

	class FramebufferSizeEvent final : public Event
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

	class WindowFocusEvent final : public Event
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

	class WindowRefreshEvent final : public Event
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

	class MouseButtonEvent final : public Event
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

	class ScrollEvent final : public Event
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

	class CursorPositionEvent final : public Event
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

	class CursorEnterEvent final : public Event
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

	class KeyEvent final : public Event
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

	class SceneChangeEvent final : public Event
	{
	public:
		SceneChangeEvent(Handle<Scene> currentScene, Handle<Scene> newScene) : CurrentScene(currentScene), NewScene(newScene)
		{
		}

		virtual std::string GetDescription() const override
		{
			return "SceneChangeEvent";
		}

		Handle<Scene> CurrentScene;
		Handle<Scene> NewScene;
	};

	class SystemStateChangeEvent final : public Event
	{
	public:
		SystemStateChangeEvent(ISystem* system, bool newState) : System(system), NewState(newState)
		{
		}

		virtual std::string GetDescription() const override
		{
			return "SystemStateChangeEvent";
		}

		ISystem* System;
		bool NewState;
	};
}