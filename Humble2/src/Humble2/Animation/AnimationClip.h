#pragma once

#include "Utilities/Collections/Span.h"

#include <glm/glm.hpp>

#include <string>

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
        std::vector<glm::vec4> outputs_raw;     // Packed: for CUBICSPLINE stores in_tan/value/out_tan triples

        // For CUBICSPLINE, we split the raw data for easier interpolation
        std::vector<glm::vec4> in_tangents;
        std::vector<glm::vec4> values;
        std::vector<glm::vec4> out_tangents;
    };

    struct AnimationClipDescriptor
    {
        const char* name;
        Span<const AnimationSampler> samplers;
        Span<const AnimationChannel> channels;
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
        }

        std::string name;
        std::vector<AnimationSampler> Samplers;
        std::vector<AnimationChannel> Channels;
        float Duration = 0.f;
	};
}