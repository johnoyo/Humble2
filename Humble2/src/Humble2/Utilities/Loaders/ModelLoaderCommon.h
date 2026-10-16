#pragma once

#include "Animation/AnimationSkeleton.h"

#include <cstdint>
#include <vector>
#include <string_view>

namespace HBL2
{
    struct SkinVertex
    {
        uint16_t Joints[4] = { 0, 0, 0, 0 };
        float Weights[4] = { 1.f, 0.f, 0.f, 0.f };
    };

    struct SkeletonImport
    {
        bool Valid = false;
        uint32_t SkinIndex = 0;                // glTF only
        std::vector<int32_t> NodeToJoint;      // glTF: node index. FBX: ufbx_node::typed_id. -1 if not a joint
        std::vector<uint16_t> SkinJointRemap;  // glTF only - skin.joints[k] (what JOINTS_0 indexes) -> final joint index
        SkeletonDescriptor Descriptor;
    };

    static uint32_t HashJointName(std::string_view s)
    {
        uint32_t h = 2166136261u;
        for (char c : s) { h ^= (uint8_t)c; h *= 16777619u; }
        return h;
    }
}
