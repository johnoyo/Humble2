#pragma once

#include "AnimationClip.h"
#include "AnimationSkeleton.h"

#include <vector>

namespace HBL2
{
	namespace Component
	{
		// Important: The attachment system runs after animation evaluation.
		struct Attachment
		{
			Entity Target;          // the character entity (owns the Animator/pose)
			int16_t JointIndex;     // hand joint or holster joint
			glm::mat4 Offset;       // socket offset, or the gun's grip offset
		};

		/*
			for (auto [gunEntity, attachment] : view<Attachment>())
			{
				const Animator& anim = Get<Animator>(attachment.Target);
				const glm::mat4& characterWorld = GetWorldMatrix(attachment.Target);

				// model-space joint transform, NOT the skinning matrix
				const glm::mat4& joint = anim.ModelSpacePose[attachment.JointIndex];

				SetWorldMatrix(gunEntity, characterWorld * joint * attachment.Offset);
			}
		*/
	}
}