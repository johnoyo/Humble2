#pragma once

#include "Enums.h"
#include "ScriptableRenderPass.h"

#include "Utilities\Collections\DynamicArray.h"

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
		DynamicArray<ScriptableRenderPass*> m_RenderPasses;
	};
}