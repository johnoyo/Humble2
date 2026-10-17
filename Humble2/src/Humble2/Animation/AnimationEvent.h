#pragma once

#include "AnimationClip.h"
#include "Core/EventDispatcher.h"

namespace HBL2
{
	struct AnimationStartedEvent final : public EventType<AnimationStartedEvent>
	{
		AnimationStartedEvent(bool looping, Handle<AnimationClip> animation)
			: IsLoopingEnabled(looping), Animation(animation)
		{
		}

		bool IsLoopingEnabled;
		Handle<AnimationClip> Animation;
	};

	struct AnimationEndedEvent final : public EventType<AnimationEndedEvent>
	{
		AnimationEndedEvent(bool looping, Handle<AnimationClip> animation)
			: IsLoopingEnabled(looping), Animation(animation)
		{
		}

		bool IsLoopingEnabled;
		Handle<AnimationClip> Animation;
	};

	struct AnimationStoppedEvent final : public EventType<AnimationStoppedEvent>
	{
		AnimationStoppedEvent(float timestamp, Handle<AnimationClip> animation)
			: Timestamp(timestamp), Animation(animation)
		{
		}

		float Timestamp;
		Handle<AnimationClip> Animation;
	};

	struct AnimationResumedEvent final : public EventType<AnimationResumedEvent>
	{
		AnimationResumedEvent(float timestamp, Handle<AnimationClip> animation)
			: Timestamp(timestamp), Animation(animation)
		{
		}

		float Timestamp;
		Handle<AnimationClip> Animation;
	};

	struct AnimationChangedEvent final : public EventType<AnimationChangedEvent>
	{
		AnimationChangedEvent(Handle<AnimationClip> oldAnimation, Handle<AnimationClip> newAnimation)
			: AnimationOld(oldAnimation), AnimationNew(newAnimation)
		{
		}

		Handle<AnimationClip> AnimationOld;
		Handle<AnimationClip> AnimationNew;
	};

	struct AnimationBlendingStartedEvent final : public EventType<AnimationBlendingStartedEvent>
	{
		AnimationBlendingStartedEvent(Handle<AnimationClip> previous, Handle<AnimationClip> current)
			: AnimationPrevious(previous), AnimationCurrent(current)
		{
		}

		Handle<AnimationClip> AnimationPrevious;
		Handle<AnimationClip> AnimationCurrent;
	};

	struct AnimationBlendingEndedEvent final : public EventType<AnimationBlendingEndedEvent>
	{
		AnimationBlendingEndedEvent(Handle<AnimationClip> current)
			: AnimationCurrent(current)
		{
		}

		Handle<AnimationClip> AnimationCurrent;
	};

	struct AnimationRagdollHandoffEvent final : public EventType<AnimationRagdollHandoffEvent>
	{
	};
}