#pragma once

#include "Humble2API.h"

#include "Enums.h"
#include "Resources\Types.h"
#include "Resources\Handle.h"
#include "DrawList.h"

namespace HBL2
{
	struct HBL2_API RenderPassContext
	{
		Handle<Shader> Shader;
		Handle<Material> Material;
		Handle<BindGroup> GlobalBindGroup;
	};

	class HBL2_API ScriptableRenderPass
	{
	public:
		virtual void Initialize() = 0;
		virtual void Execute() = 0;
		virtual void Destroy() = 0;

		const RenderPassEvent& GetInjectionPoint() const { return m_InjectionPoint; }

	protected:
		DrawList GetDraws();
		RenderPassContext CreateContext(const char* shaderPath, Handle<RenderPass> renderPass);

	protected:
		RenderPassContext m_RenderPassContext;
		RenderPassEvent m_InjectionPoint = RenderPassEvent::AfterRenderingPostProcess;
		const char* m_PassName = "";
	};

	/*

	class DitherRenderPass final : public ScriptableRenderPass
	{
	public:
		virtual void Initialize() override
		{
			m_PassName = "DitherRenderPass";
			m_InjectionPoint = RenderPassEvent::AfterRenderingPostProcess;
			m_RenderPassContext = CreateContext("assets/shaders/post-process-dithering.shader", m_RenderPass);
		}

		virtual void Execute() override
		{
			RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_RenderPass, m_FrameBuffer);
			GlobalDrawStream globalDrawStream = { .BindGroup = m_RenderPassContext.GlobalBindGroup };
			passRenderer->DrawSubPass(globalDrawStream, GetDraws());
			commandBuffer->EndRenderPass(*passRenderer);
		}

		virtual void Destroy() override
		{
		}

	private:
		Handle<RenderPass> m_RenderPass;
		Handle<FrameBuffer> m_FrameBuffer;
	};

	extern "C" __declspec(dllexport) void RegisterDitherRenderPass(HBL2::RenderPassPool* pool)
	{
		DitherRenderPass* newDitherRenderPass = new DitherRenderPass;
		pool->AddRenderPass(newDitherRenderPass);
	}

	*/
}