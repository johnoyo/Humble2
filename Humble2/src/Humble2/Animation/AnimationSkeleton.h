#pragma once

#include "Resources/Handle.h"
#include "Resources/BaseTypeDefinitions.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace HBL2
{
    struct JointTransform
    {
        glm::vec3 T;
        glm::quat R;
        glm::vec3 S;
    };

    struct Pose
    {
        std::vector<JointTransform> Local;
        std::vector<glm::mat4> Model;
        std::vector<glm::mat4> Skinning;   // Model * InverseBind, what the GPU gets
        Handle<Buffer> BoneBuffer; // or an offset into a shared per-frame buffer
    };

    struct SkeletonDescriptor
    {

    };

    // Resource: shared, immutable
    struct Skeleton
    {
        Skeleton(const SkeletonDescriptor&& desc)
        {

        }

        std::vector<int16_t> Parents;       // -1 = root, parent index < child index
        std::vector<uint32_t> NameHashes;
        std::vector<std::string> Names;         // debug / editor
        std::vector<glm::mat4> InverseBind;
        std::vector<JointTransform> BindPose;
    };
}