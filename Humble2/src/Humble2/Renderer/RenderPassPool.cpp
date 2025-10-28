#include "RenderPassPool.h"

namespace HBL2
{
	void RenderPassPool::AddRenderPass(ScriptableRenderPass* renderPass)
	{
		m_RenderPasses.Add(renderPass);
	}

	void RenderPassPool::Initialize()
	{
		for (const auto& renderPass : m_RenderPasses)
		{
			renderPass->Initialize();
		}
	}

	void RenderPassPool::Execute(RenderPassEvent event)
	{
		for (const auto& renderPass : m_RenderPasses)
		{
			if (renderPass->GetInjectionPoint() == event)
			{
				renderPass->Execute();
				return;
			}
		}
	}

	void RenderPassPool::Destroy()
	{
		for (const auto& renderPass : m_RenderPasses)
		{
			renderPass->Destroy();
		}
	}
}

