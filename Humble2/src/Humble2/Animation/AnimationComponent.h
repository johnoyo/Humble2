#pragma once

#include "AnimationClip.h"
#include "AnimationSkeleton.h"

#include <vector>

namespace HBL2
{
	namespace Component
	{
		struct Animation
		{
			int DefaultClip = 0;
			int CurrentClip = -1;
			int ClipsCount = 0;

			float Time = 0.f;
			float Speed = 1.f;
			float TransitionDuration = 0.2f;

			bool EnableLooping;
			bool PlayAutomatically;
			bool ApplyRootMotion;
			bool Enabled = true;

			bool LastEnabled = true;
			int LastClip = -1;
			float BlendElapsed = 0.0f;
			float BlendDuration = 0.0f;
			std::vector<JointTransform> BlendFrom;
		};

		struct SkeletonPose
		{
			RefHandle<Asset> Mesh;

			std::vector<JointTransform> Local;
			std::vector<glm::mat4> Model;
			std::vector<glm::mat4> Skinning;
		};

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