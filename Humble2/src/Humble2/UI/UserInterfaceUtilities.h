#pragma once

#include "Editor.h"

#include "imgui.h"

namespace HBL2
{
	namespace UI
	{
		namespace Utils
		{
			static inline ImVec2 GetViewportSize()
			{
				return *(ImVec2*)&Context::ViewportSize;
			}

			static inline ImVec2 GetViewportPosition()
			{
				return *(ImVec2*)&Context::ViewportPosition;
			}

			static inline float GetFontSize()
			{
				return ImGui::GetFontSize();
			}
		}
	}

	class EditorUtilities
	{
	public:
		EditorUtilities(const EditorUtilities&) = delete;

		static EditorUtilities& Get()
		{
			static EditorUtilities instance;
			return instance;
		}

		template<typename C>
		bool HasCustomEditor()
		{
			return m_CustomEditors.find(typeid(C).hash_code()) != m_CustomEditors.end();
		}

		template<typename C, typename E>
		bool DrawCustomEditor(const C& component)
		{
			E* customEditor = (E*)m_CustomEditors[typeid(C).hash_code()];
			customEditor->OnUpdate(component);
			return customEditor->GetRenderBaseEditor();
		}

		template<typename C, typename E>
		void InitCustomEditor()
		{
			E* customEditor = (E*)m_CustomEditors[typeid(C).hash_code()];
			customEditor->OnCreate();
		}

		template<typename C, typename E>
		void RegisterCustomEditor()
		{
			m_CustomEditors[typeid(C).hash_code()] = new E;
		}

	private:
		EditorUtilities() = default;

		std::unordered_map<size_t, void*> m_CustomEditors;
	};
}