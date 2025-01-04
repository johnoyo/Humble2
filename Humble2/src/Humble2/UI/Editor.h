#pragma once

#include "Base.h"

template<typename T, typename C>
class Editor
{
public:
	virtual ~Editor() = default;

	void OnCreate()
	{
		static_cast<T*>(this)->OnCreate();
	}

	void OnUpdate(const C& component)
	{
		static_cast<T*>(this)->OnUpdate(component);
	}

	bool GetRenderBaseEditor() const
	{
		return m_RenderBaseEditor;
	}

protected:
	Editor() = default;

	bool m_RenderBaseEditor = false;
};
