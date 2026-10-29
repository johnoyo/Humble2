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
        std::vector<std::string> Names;

        // Per-joint inverse of the joint's model-space transform in the bind pose (the pose the mesh was authored in).
        // Computed once at import and never changes at runtime.
        // Moves a bind-pose vertex into the joint's local frame, so that multiplying by the animated
        // pose.Model[i] carries it to its current position: Skinning[i] = Model[i] * InverseBind[i].
        // With the pose equal to the bind pose, Skinning[i] is the identity matrix.
        // 
        // NOTE: The joint’s local frame is the coordinate system with its origin at the joint and its axes pointing the way the joint points,
        // so it moves and rotates with the joint.
        std::vector<glm::mat4> InverseBind;

        // Per-joint local transform (relative to the parent) in the rest pose the mesh was authored in.
        // Starting point for every pose, so joints without animation channels stay valid.
        std::vector<JointTransform> BindPose;

        glm::mat4 RootTransform = glm::mat4(1.0f);
    };

    
}