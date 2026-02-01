#pragma once

#include "Enums.h"
#include "ScriptableRenderPass.h"

#include <vector>

namespace HBL2
{
	class RenderPassPool
	{
	public:
		void AddRenderPass(ScriptableRenderPass* renderPass);
		void Initialize();
		void Execute(RenderPassEvent event);
		void Destroy();

	private:
		std::vector<ScriptableRenderPass*> m_RenderPasses;
	};
}