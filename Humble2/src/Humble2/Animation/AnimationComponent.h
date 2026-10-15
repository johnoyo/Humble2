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
			Handle<Asset> Mesh;

			std::vector<Handle<AnimationClip>> Clips;

			int DefaultClip = 0;
			int CurrentClip = -1;

			float Time = 0.f;
			float Speed = 1.f;

			bool EnableLooping;
			bool PlayAutomatically;
			bool ApplyRootMotion;
			bool Enabled;

			uint32_t JointCount = 0;

			Pose Pose;
		};

		// Important: The attachment system runs after animation evaluation.
		struct Attachment
		{
			Entity Target;          // the character entity (owns the Animator/pose)
			int16_t JointIndex;     // hand joint or holster joint
			glm::mat4 Offset;       // socket offset, or the gun's grip offset
		};
	}
}