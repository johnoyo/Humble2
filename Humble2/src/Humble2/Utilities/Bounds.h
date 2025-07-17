#pragma once

#include "glm/glm.hpp"

#include <algorithm>
#include <limits>

struct Bounds {
    glm::vec3 Center{ 0.0f };
    glm::vec3 Extents{ 0.0f };             // half-size (always ≥ 0)

    constexpr Bounds() = default;

    constexpr Bounds(const glm::vec3& c, const glm::vec3& size)
        : Center(c), Extents(size * 0.5f)
    {
    }

    static constexpr Bounds FromMinMax(const glm::vec3& mn,
        const glm::vec3& mx)
    {
        return { (mn + mx) * 0.5f, mx - mn };
    }

    constexpr glm::vec3 size() const { return Extents * 2.0f; }
    constexpr glm::vec3 min() const { return Center - Extents; }
    constexpr glm::vec3 max() const { return Center + Extents; }
    constexpr float volume() const
    {
        glm::vec3 s = size(); return s.x * s.y * s.z;
    }

    void SetMinMax(const glm::vec3& mn, const glm::vec3& mx)
    {
        Center = (mn + mx) * 0.5f;
        Extents = mx - Center;
    }

    void Encapsulate(const glm::vec3& pt)
    {
        glm::vec3 mn = min();
        glm::vec3 mx = max();
        mn = glm::min(mn, pt);
        mx = glm::max(mx, pt);
        SetMinMax(mn, mx);
    }

    void Encapsulate(const Bounds& b)
    {
        glm::vec3 mn = glm::min(min(), b.min());
        glm::vec3 mx = glm::max(max(), b.max());
        SetMinMax(mn, mx);
    }

    void Expand(float amount) { Extents += glm::vec3(amount * 0.5f); }
    void Expand(const glm::vec3& diff) { Extents += diff * 0.5f; }

    bool Contains(const glm::vec3& pt) const
    {
        glm::vec3 d = glm::abs(pt - Center);
        return (d.x <= Extents.x &&
            d.y <= Extents.y &&
            d.z <= Extents.z);
    }

    bool Contains(const Bounds& b) const
    {
        return Contains(b.min()) && Contains(b.max());
    }

    bool Intersects(const Bounds& b) const
    {
        glm::vec3 d = glm::abs(b.Center - Center);
        glm::vec3 sum = b.Extents + Extents;
        return (d.x <= sum.x &&
            d.y <= sum.y &&
            d.z <= sum.z);
    }

    // slab method — returns true if ray hits the box; tNear/out optional
    bool IntersectRay(const glm::vec3& ro, const glm::vec3& rd, float* tNear = nullptr, float* tFar = nullptr) const
    {
        const glm::vec3 invDir = 1.0f / rd;

        glm::vec3 t0 = (min() - ro) * invDir;
        glm::vec3 t1 = (max() - ro) * invDir;

        // swap to make t0 = near, t1 = far
        if (t0.x > t1.x) std::swap(t0.x, t1.x);
        if (t0.y > t1.y) std::swap(t0.y, t1.y);
        if (t0.z > t1.z) std::swap(t0.z, t1.z);

        float tn = std::max({ t0.x, t0.y, t0.z });
        float tf = std::min({ t1.x, t1.y, t1.z });

        if (tf < 0.0f || tn > tf) return false;   // behind or miss

        if (tNear) *tNear = tn;
        if (tFar)  *tFar = tf;
        return true;
    }

    glm::vec3 ClosestPoint(const glm::vec3& pt) const
    {
        return glm::clamp(pt, min(), max());
    }

    float SqrDistance(const glm::vec3& pt) const
    {
        glm::vec3 q = ClosestPoint(pt) - pt;
        return glm::dot(q, q);
    }

    friend bool operator==(const Bounds& a, const Bounds& b)
    {
        return a.Center == b.Center && a.Extents == b.Extents;
    }

    friend bool operator!=(const Bounds& a, const Bounds& b)
    {
        return !(a == b);
    }
};