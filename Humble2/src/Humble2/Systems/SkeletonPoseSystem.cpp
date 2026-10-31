#include "SkeletonPoseSystem.h"

#include "Asset/AssetManager.h"
#include "Resources/ResourceManager.h"
#include "Utilities/ShaderUtilities.h"

namespace HBL2
{
	static glm::mat4 ToMat4(const JointTransform& tr)
	{
		glm::mat4 T = glm::translate(glm::mat4(1.0f), tr.T);
		glm::mat4 R = glm::mat4_cast(tr.R);
		glm::mat4 S = glm::scale(glm::mat4(1.0f), tr.S);

		return T * R * S;
	}

	void SkeletonPoseSystem::OnCreate()
	{

	}

	void SkeletonPoseSystem::OnUpdate(float ts)
	{
		m_Context->Filter<Component::SkeletonPose>()
			.ForEach([](Entity entity, Component::SkeletonPose& pose)
			{
				Handle<Mesh> meshHandle = AssetManager::Instance->GetAsset<Mesh>(pose.Mesh.Get());
				Mesh* mesh = ResourceManager::Instance->GetMesh(meshHandle);
				if (!mesh)
				{
					return;
				}

				const Skeleton* skel = ResourceManager::Instance->GetSkeleton(mesh->Skeleton);
				if (!skel)
				{
					return;
				}

				// Convert from local to world (turn the final local pose into matrices).
				for (size_t i = 0; i < skel->Parents.size(); ++i)
				{
					const glm::mat4 local = ToMat4(pose.Local[i]);
					pose.Model[i] = skel->Parents[i] < 0 ? skel->RootTransform * local : pose.Model[skel->Parents[i]] * local;
					pose.Skinning[i] = pose.Model[i] * skel->InverseBind[i];
				}
			});
	}

	void SkeletonPoseSystem::OnDestroy()
	{
		m_Context->Filter<Component::SkeletonPose>()
			.ForEach([](Entity entity, Component::SkeletonPose& pose)
			{
				pose.Mesh.Release();
			});
	}
}
