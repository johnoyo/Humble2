#pragma once

#include "EventDispatcher.h"
#include "Resources\Handle.h"

namespace HBL2
{
	class Scene;
	class ISystem;
	enum class SystemState;

	class HBL2_API WindowCloseEvent final : public EventType<WindowCloseEvent>
	{
	public:
		WindowCloseEvent()
		{
		}
	};

	class HBL2_API WindowSizeEvent final : public EventType<WindowSizeEvent>
	{
	public:
		WindowSizeEvent(int width, int height) : Width(width), Height(height)
		{
		}

		int Width = 0;
		int Height = 0;
	};

	class HBL2_API FramebufferSizeEvent final : public EventType<FramebufferSizeEvent>
	{
	public:
		FramebufferSizeEvent(int width, int height) : Width(width), Height(height)
		{
		}

		int Width = 0;
		int Height = 0;
	};

	class HBL2_API ViewportSizeEvent final : public EventType<ViewportSizeEvent>
	{
	public:
		ViewportSizeEvent(int width, int height) : Width(width), Height(height)
		{
		}

		int Width = 0;
		int Height = 0;
	};

	class HBL2_API WindowFocusEvent final : public EventType<WindowFocusEvent>
	{
	public:
		WindowFocusEvent(int focused) : Focused(focused)
		{
		}

		int Focused = 0;
	};

	class HBL2_API WindowRefreshEvent final : public EventType<WindowRefreshEvent>
	{
	public:
		WindowRefreshEvent()
		{
		}
	};

	class HBL2_API MouseButtonEvent final : public EventType<MouseButtonEvent>
	{
	public:
		MouseButtonEvent(int button, int action, int mods) : Button(button), Action(action), Mods(mods)
		{
		}

		int Button;
		int Action;
		int Mods;
	};

	class HBL2_API ScrollEvent final : public EventType<ScrollEvent>
	{
	public:
		ScrollEvent(double xoffset, double yoffset) : XOffset(xoffset), YOffset(yoffset)
		{
		}

		double XOffset;
		double YOffset;
	};

	class HBL2_API CursorPositionEvent final : public EventType<CursorPositionEvent>
	{
	public:
		CursorPositionEvent(double xpos, double ypos) : XPosition(xpos), YPosition(ypos)
		{
		}

		double XPosition;
		double YPosition;
	};

	class HBL2_API CursorEnterEvent final : public EventType<CursorEnterEvent>
	{
	public:
		CursorEnterEvent(int entered) : Entered(entered)
		{
		}

		int Entered;
	};

	class HBL2_API KeyEvent final : public EventType<KeyEvent>
	{
	public:
		KeyEvent(int key, int scancode, int action, int mods) : Key(key), Scancode(scancode), Action(action), Mods(mods)
		{
		}

		int Key;
		int Scancode;
		int Action;
		int Mods;
	};

	class HBL2_API SceneChangeEvent final : public EventType<SceneChangeEvent>
	{
	public:
		SceneChangeEvent(Handle<Scene> oldScene, Handle<Scene> newScene) : OldScene(oldScene), NewScene(newScene)
		{
		}

		Handle<Scene> OldScene;
		Handle<Scene> NewScene;
	};

	class HBL2_API SystemStateChangeEvent final : public EventType<SystemStateChangeEvent>
	{
	public:
		SystemStateChangeEvent(ISystem* system, SystemState oldState, SystemState newState) : System(system), OldState(oldState), NewState(newState)
		{
		}

		ISystem* System;
		SystemState OldState;
		SystemState NewState;
	};
}