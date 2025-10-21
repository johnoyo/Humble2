#pragma once

#include "Renderer.h"
#include "DrawList.h"

#include "Resources/ResourceManager.h"

#include <glm/glm.hpp>

namespace HBL2
{
	namespace Color
	{
		constexpr uint32_t White = 0xFFFFFFFF;
		constexpr uint32_t Black = 0xFF000000;
		constexpr uint32_t Red = 0xFF0000FF;
		constexpr uint32_t Green = 0xFF00FF00;
		constexpr uint32_t Blue = 0xFFFF0000;
		constexpr uint32_t Yellow = 0xFF00FFFF;
		constexpr uint32_t Cyan = 0xFFFFFF00;
		constexpr uint32_t Magenta = 0xFFFF00FF;
	}

	class HBL2_API DebugRenderer
	{
	public:
		static DebugRenderer* Instance;

		DebugRenderer() = default;
		virtual ~DebugRenderer() = default;

		virtual void Initialize();
		virtual void BeginFrame();
		virtual void EndFrame();
		virtual void Flush(CommandBuffer* commandBuffer);
		virtual void Clean();

		// Public API
		uint32_t Color = Color::White;

		virtual void DrawLine(const glm::vec3& from, const glm::vec3& to);
		virtual void DrawRay(const glm::vec3& from, const glm::vec3& direction);
		virtual void DrawSphere(const glm::vec3& position, float radius);
		virtual void DrawWireSphere(const glm::vec3& position, float radius);
		virtual void DrawCube(const glm::vec3& center, const glm::vec3& size);
		virtual void DrawWireCube(const glm::vec3& center, const glm::vec3& size);

	private:
		glm::mat4 GetCameraMVP();

	private:
		static constexpr uint32_t s_MaxDebugVertices = 1'000'000; // 1 million vertices = ~16MB

		struct DebugVertex
		{
			glm::vec3 Position;
			uint32_t Color;
		};

		ResourceManager* m_ResourceManager = nullptr;

		Handle<Shader> m_DebugShader;
		Handle<Material> m_DebugLineMaterial;
		uint64_t m_DebugLineMaterialVariantHash = UINT64_MAX;

		Handle<RenderPass> m_DebugRenderPass;
		Handle<RenderPassLayout> m_DebugRenderPassLayout;
		Handle<FrameBuffer> m_DebugFrameBuffer;

		Handle<Buffer> m_LineVertexBuffer;

		DrawList m_Draws;

		std::vector<DebugVertex> m_LineVerts;
		uint32_t m_CurrentLineIndex = 0;

		std::vector<DebugVertex> m_FillTrisVerts;
		std::vector<uint32_t>  m_FillTrisIndices;

		std::vector<DebugVertex> m_WireTrisVerts;
		std::vector<uint32_t>  m_WireTrisIndices;
	};
}