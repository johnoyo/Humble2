#include "AnimationSystem.h"

#include "Asset/AssetManager.h"
#include "Resources/ResourceManager.h"
#include "Utilities/ShaderUtilities.h"

namespace HBL2
{
	struct KeyframeSpan
	{
		uint32_t K0 = 0, K1 = 0;
		float T = 0.f;    // 0..1 between K0 and K1
		float Dt = 0.f;   // K1 time - K0 time, needed for cubic spline tangents
	};

	static KeyframeSpan FindSpan(const std::vector<float>& times, float time)
	{
		const uint32_t n = (uint32_t)times.size();

		// before the first key, after the last key, or a single key: clamp
		if (n == 1 || time <= times.front())
		{
			return { 0, 0, 0.f, 0.f };
		}

		if (time >= times.back())
		{
			return { n - 1, n - 1, 0.f, 0.f };
		}

		// first key with time > requested time, so K0 is the key just before it
		const uint32_t k1 = (uint32_t)(std::upper_bound(times.begin(), times.end(), time) - times.begin());
		const uint32_t k0 = k1 - 1;
		const float dt = times[k1] - times[k0];

		return { k0, k1, (time - times[k0]) / dt, dt };
	}

	static glm::quat ToQuat(const glm::vec4& v)
	{
		return glm::quat(v.w, v.x, v.y, v.z);   // glTF is xyzw, glm constructor is wxyz
	}

	static glm::vec4 Evaluate(const AnimationSampler& s, const KeyframeSpan& span, bool isRotation)
	{
		const glm::vec4& v0 = s.values[span.K0];

		// clamped (before first / after last / single key)
		if (span.K0 == span.K1) return v0;

		const glm::vec4& v1 = s.values[span.K1];
		const float t = span.T;

		switch (s.interpolation)
		{
		case AnimationSampler::STEP:
			{
				return v0;
			}
		case AnimationSampler::LINEAR:
			{
				if (isRotation)
				{
					const glm::quat q = glm::slerp(ToQuat(v0), ToQuat(v1), t);
					return { q.x, q.y, q.z, q.w };
				}
				return glm::mix(v0, v1, t);
			}
		case AnimationSampler::CUBICSPLINE:
			{
				// Hermite spline: value at K0, out-tangent of K0, value at K1, in-tangent of K1
				const glm::vec4& b0 = s.out_tangents[span.K0];
				const glm::vec4& a1 = s.in_tangents[span.K1];

				const float t2 = t * t;
				const float t3 = t2 * t;

				glm::vec4 r =
					(2.f * t3 - 3.f * t2 + 1.f) * v0 +
					span.Dt * (t3 - 2.f * t2 + t) * b0 +
					(-2.f * t3 + 3.f * t2) * v1 +
					span.Dt * (t3 - t2) * a1;

				if (isRotation)
				{
					r = glm::normalize(r);   // spline result isn't unit length
				}

				return r;
			}
		}
		return v0;
	}

	void SampleClip(const AnimationClip* clip, float time, std::vector<JointTransform>& pose)
	{
		for (const AnimationChannel& ch : clip->Channels)
		{
			const AnimationSampler& s = clip->Samplers[ch.SamplerIndex];

			if (s.inputs.empty())
			{
				continue;
			}

			const KeyframeSpan span = FindSpan(s.inputs, time);
			JointTransform& joint = pose[ch.JointIndex];

			switch (ch.Path)
			{
			case AnimationChannel::TRANSLATION:
				{
					joint.T = glm::vec3(Evaluate(s, span, false));
					break;
				}
			case AnimationChannel::ROTATION:
				{
					const glm::vec4 v = Evaluate(s, span, true);
					joint.R = glm::normalize(ToQuat(v));
					break;
				}
			case AnimationChannel::SCALE:
				{
					joint.S = glm::vec3(Evaluate(s, span, false));
					break;
				}
			case AnimationChannel::WEIGHTS:   // morph targets, later
				{
					break;
				}
			}
		}
	}

	void AnimationSystem::OnAttach()
	{
	}

	void AnimationSystem::OnCreate()
	{
	}

	// https://github.com/KhronosGroup/Vulkan-Tutorial/blob/main/attachments/advanced_gltf/animation_component.cpp

	void AnimationSystem::OnUpdate(float ts)
	{
		BEGIN_PROFILE_SYSTEM();

		m_Context->Filter<Component::Animation, Component::SkeletonPose>()
			.ForEach([this, ts](Entity entity, Component::Animation& anim, Component::SkeletonPose& pose)
			{
				if (!anim.Enabled || anim.CurrentClip < 0)
				{
					return;
				}

				// Retrieve mesh.
				Handle<Mesh> meshHandle = AssetManager::Instance->GetAsset<Mesh>(pose.Mesh.Get());
				Mesh* mesh = ResourceManager::Instance->GetMesh(meshHandle);

				if (mesh == nullptr)
				{
					return;
				}

				// Retrieve skeleton and current animation clip.
				Skeleton* skel = ResourceManager::Instance->GetSkeleton(mesh->Skeleton);
				AnimationClip* clip = ResourceManager::Instance->GetAnimationClip(mesh->AnimationClips[anim.CurrentClip]);

				// Advance animation time.
				anim.Time += ts * anim.Speed;
				if (anim.Time > clip->Duration)
				{
					anim.Time = anim.EnableLooping ? fmod(anim.Time, clip->Duration) : clip->Duration;
				}

				// Start from bind pose so joints without channels stay valid.
				pose.Local = skel->BindPose;
				pose.Model.resize(skel->Parents.size());
				pose.Skinning.resize(skel->Parents.size());

				// Sample animation clip.
				SampleClip(clip, anim.Time, pose.Local);
			});

		END_PROFILE_SYSTEM(RunningTime);
	}

	void AnimationSystem::OnDestroy()
	{
	}
}
