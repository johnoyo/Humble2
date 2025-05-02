#include "VulkanComputePassRenderer.h"

#include "VulkanResourceManager.h"

namespace HBL2
{
	void VulkanComputePassRenderer::Dispatch(const Span<const HBL2::Dispatch>& dispatches)
	{
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

		for (const auto& dispatch : dispatches)
		{
			VulkanShader* shader = rm->GetShader(dispatch.Shader);
			VulkanBindGroup* bindGroup = rm->GetBindGroup(dispatch.BindGroup);

			vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, shader->GetOrCreateVariant(dispatch.Variant));
			vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, shader->PipelineLayout, 0, 1, &bindGroup->DescriptorSet, 0, nullptr);

			vkCmdDispatch(m_CommandBuffer, dispatch.ThreadGroupCount.x, dispatch.ThreadGroupCount.y, dispatch.ThreadGroupCount.z);
		}
	}
}