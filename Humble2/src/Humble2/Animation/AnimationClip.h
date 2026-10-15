#pragma once

#include "AnimationSkeleton.h"

#include "Utilities/Collections/Span.h"

#include <glm/glm.hpp>

namespace HBL2
{
    // Structure for animation keyframes
    struct AnimationChannel
    {
        enum PathType { TRANSLATION, ROTATION, SCALE, WEIGHTS };

        PathType Path;
        uint16_t JointIndex;
        uint32_t SamplerIndex;
    };

    // Structure for animation interpolation
    struct AnimationSampler
    {
        enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
        InterpolationType interpolation = LINEAR;
        std::vector<float> inputs;              // Timestamps in seconds

        // For CUBICSPLINE, we split the raw data for easier interpolation
        std::vector<glm::vec4> in_tangents;
        std::vector<glm::vec4> values;
        std::vector<glm::vec4> out_tangents;
    };

    struct AnimationClipDescriptor
    {
        std::string name;
        std::vector<AnimationSampler> samplers;
        std::vector<AnimationChannel> channels;
        float duration = 0.0f;

        Handle<Skeleton> skeleton;
    };

    // Structure for animation clip
    struct AnimationClip
	{
        AnimationClip(const AnimationClipDescriptor&& desc)
        {
            name = desc.name;

            for (const auto& sampler : desc.samplers)
            {
                Samplers.push_back(sampler);
            }

            for (const auto& channel : desc.channels)
            {
                Channels.push_back(channel);
            }

            Duration = desc.duration;
        }

        std::string name;
        std::vector<AnimationSampler> Samplers;
        std::vector<AnimationChannel> Channels;
        float Duration = 0.f;
	};
}