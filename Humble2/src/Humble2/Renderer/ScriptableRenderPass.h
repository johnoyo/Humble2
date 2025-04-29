#pragma once

#include "Enums.h"

namespace HBL2
{
	class ScriptableRenderPass
	{
	public:
		virtual void Initialize() = 0;
		virtual void Execute() = 0;
		virtual void Destroy() = 0;

		const RenderPassEvent& GetInjectionPoint() const { return m_InjectionPoint; }

	protected:
		RenderPassEvent m_InjectionPoint;
		const char* m_PassName;
	};

	/*
	* 
	class DitherRenderPass : public ScriptableRenderPass
	{
	public:
		virtual void Initialize() override
		{
			m_PassName = "DitherRenderPass";
			m_InjectionPoint = RenderPassEvent::AfterRenderingPostProcess;
		}

		virtual void Execute() override
		{
			RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_RenderPass, m_FrameBuffer);

			GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings };
			passRenderer->DrawSubPass(globalDrawStream, draws);
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