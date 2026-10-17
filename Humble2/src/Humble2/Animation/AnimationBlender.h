#pragma once

#include "Base.h"
#include "AnimationComponent.h"

#include "Utilities/Collections/Span.h"

namespace HBL2
{
	class AnimationBlender
	{
	public:
		struct LinearBlender
		{
			inline static glm::quat BlendRotation(const glm::quat& quat0, const glm::quat& quat1, float t)
			{
				return glm::slerp(quat0, quat1, t);
			}

			inline static glm::vec3 BlendTranslation(const glm::vec3& trans0, const glm::vec3& trans1, float t)
			{
				return glm::mix(trans0, trans1, t);
			}

			inline static glm::vec3 BlendScale(const glm::vec3& scale0, const glm::vec3& scale1, float t)
			{
				return glm::mix(scale0, scale1, t);
			}
		};

		struct AdditiveBlender
		{
			inline static glm::quat BlendRotation(const glm::quat& quat0, const glm::quat& quat1, float t)
			{
				glm::quat targetQuat = quat0 * quat1;
				return glm::slerp(quat0, targetQuat, t);
			}

			inline static glm::vec3 BlendTranslation(const glm::vec3& trans0, const glm::vec3& trans1, float t)
			{
				return trans1 * glm::vec3(t) + trans0;
			}

			inline static glm::vec3 BlendScale(const glm::vec3& scale0, const glm::vec3& scale1, float t)
			{
				return scale1 * glm::vec3(t) + scale0;
			}
		};

		template<typename Blender>
		static void BlendLocal(Span<const JointTransform> sourcePose, Span<const JointTransform> targetPose, float blendWeight, Span<JointTransform> outPose)
		{
			HBL2_ASSERT(sourcePose.size() == targetPose.size() && sourcePose.size() == outPose.size(), "Poses must have the same joint count.");

			blendWeight = glm::clamp(blendWeight, 0.0f, 1.0f);

			// Perform blend.
			for (uint32_t boneIdx = 0; boneIdx < outPose.size(); boneIdx++)
			{
				JointTransform outTransform = {};
				const JointTransform& sourceTransform = sourcePose[boneIdx];
				const JointTransform& targetTransform = targetPose[boneIdx];

				// Blend translations.
				outTransform.T = Blender::BlendTranslation(sourceTransform.T, targetTransform.T, blendWeight);

				// Blend scales.
				outTransform.S = Blender::BlendScale(sourceTransform.S, targetTransform.S, blendWeight);
				
				// Blend rotations.
				outTransform.R = glm::normalize(Blender::BlendRotation(sourceTransform.R, targetTransform.R, blendWeight));

				outPose[boneIdx] = outTransform;
			}
		}

		static void LinearBlendLocal(Span<const JointTransform> sourcePose, Span<const JointTransform> targetPose, float blendWeight, Span<JointTransform> outPose)
		{
			BlendLocal<LinearBlender>(sourcePose, targetPose, blendWeight, outPose);
		}

		static void AdditiveBlendLocal(Span<const JointTransform> sourcePose, Span<const JointTransform> targetPose, float blendWeight, Span<JointTransform> outPose)
		{
			BlendLocal<AdditiveBlender>(sourcePose, targetPose, blendWeight, outPose);
		}
	};
}