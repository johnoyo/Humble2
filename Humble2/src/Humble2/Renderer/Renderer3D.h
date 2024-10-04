#pragma once

#include "Shader.h"
#include "Texture.h"
#include "VertexArray.h"
#include "FrameBuffer.h"
#include "RenderCommand.h"

#include "Core/Context.h"

#include "Scene/Components.h"

#include "Renderer2D.h"
#include "RenderCommand.h"

namespace HBL2
{
	class Renderer3D
	{
	public:
		Renderer3D(const Renderer3D&) = delete;
		
		static Renderer3D& Get()
		{
			static Renderer3D instance;
			return instance;
		}

		void Initialize();

		void BeginFrame();
		void SetupMesh(Component::Transform& transform, Component::StaticMesh& mesh, glm::mat4& mvp);
		void SubmitMesh(Component::Transform& transform, Component::StaticMesh& mesh);
		void CleanMesh(Component::StaticMesh& mesh);
		void EndFrame();

		void Clean();

	private:
		Renderer3D() {}

		bool LoadFromObj(Component::StaticMesh& mesh);
		HBL::FrameBuffer* m_FrameBuffer = nullptr;
		std::vector<std::string> m_Shaders;
	};
}