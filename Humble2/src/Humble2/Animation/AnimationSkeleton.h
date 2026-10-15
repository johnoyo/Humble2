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
        const char* debugName = "";
        std::vector<int16_t> parents;
        std::vector<uint32_t> nameHashes;
        std::vector<std::string> names;
        std::vector<glm::mat4> inverseBind;
        std::vector<JointTransform> bindPose;
        glm::mat4 rootTransform = glm::mat4(1.0f);
    };

    // Resource: shared, immutable
    struct Skeleton
    {
        Skeleton(const SkeletonDescriptor&& desc)
        {
            Parents = desc.parents;
            NameHashes = desc.nameHashes;
            Names = desc.names;
            InverseBind = desc.inverseBind;
            BindPose = desc.bindPose;
            RootTransform = desc.rootTransform;
        }

        std::vector<int16_t> Parents;       // -1 = root, parent index < child index
        std::vector<uint32_t> NameHashes;
        std::vector<std::string> Names;         // debug / editor
        std::vector<glm::mat4> InverseBind;
        std::vector<JointTransform> BindPose;
        glm::mat4 RootTransform = glm::mat4(1.0f); // static non-joint ancestors of the root joints
    };
}