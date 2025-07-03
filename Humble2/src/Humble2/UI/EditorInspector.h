#pragma once

#include "Base.h"

namespace HBL2
{
	template<typename T, typename C>
	class EditorInspector
	{
	public:
		virtual ~EditorInspector() = default;

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
		EditorInspector() = default;

		bool m_RenderBaseEditor = false;
	};
}
