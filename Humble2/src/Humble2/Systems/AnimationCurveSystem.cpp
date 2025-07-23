#include "AnimationCurveSystem.h"

#include <UI\UserInterfaceUtilities.h>
#include <UI\AnimationCurveEditor.h>

namespace HBL2
{
	static inline float Hermite(float p0, float m0, float p1, float m1, float u)
	{
		float u2 = u * u;
		float u3 = u2 * u;
		float h00 = 2.0f * u3 - 3.0f * u2 + 1.0f;
		float h10 = u3 - 2.0f * u2 + u;
		float h01 = -2.0f * u3 + 3.0f * u2;
		float h11 = u3 - u2;

		return h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1;
	}

	static inline void ComputeCatmullTangents(std::vector<Component::AnimationCurve::KeyFrame>& ks)
	{
		const std::size_t n = ks.size();

		if (n < 2)
		{
			return;
		}

		for (std::size_t i = 0; i < n; ++i)
		{
			Component::AnimationCurve::KeyFrame& k = ks[i];

			float dt, dv;
			if (i == 0)
			{    
				dt = ks[1].Time - k.Time;
				dv = ks[1].Value - k.Value;
			}
			else if (i == n - 1)
			{
				dt = k.Time - ks[i - 1].Time;
				dv = k.Value - ks[i - 1].Value;
			}
			else
			{
				dt = ks[i + 1].Time - ks[i - 1].Time;
				dv = ks[i + 1].Value - ks[i - 1].Value;
			}
			k.InTan = k.OutTan = (dt != 0.0f) ? dv / dt : 0.0f;
		}
	}

	void AnimationCurveSystem::OnCreate()
	{
		m_Context->View<Component::AnimationCurve>()
			.Each([&](Component::AnimationCurve& curve)
			{
				if (curve.Keys.empty())
				{
					SetPreset(curve);
				}

				RecalculateTangents(curve);
			});

		EditorUtilities::Get().RegisterCustomEditor<Component::AnimationCurve, AnimationCurveEditor>();
		EditorUtilities::Get().InitCustomEditor<Component::AnimationCurve, AnimationCurveEditor>();
	}

	void AnimationCurveSystem::OnUpdate(float ts)
	{
		BEGIN_PROFILE_SYSTEM();

		m_Context->View<Component::AnimationCurve>()
			.Each([&](Component::AnimationCurve& curve)
			{
				if (curve.Keys.empty())
				{
					SetPreset(curve);
				}

				if (curve.Preset != curve.PrevPreset)
				{
					SetPreset(curve);
					curve.PrevPreset = curve.Preset;
				}

				if (curve.RecalculateTangents)
				{
					RecalculateTangents(curve);
					curve.RecalculateTangents = false;
				}
			});

		END_PROFILE_SYSTEM(RunningTime);
	}

	float AnimationCurveSystem::Evaluate(const Component::AnimationCurve& curve, float t)
	{
		if (curve.Keys.empty())
		{
			return 0.0f;
		}

		t = glm::clamp(t, 0.0f, 1.0f);
		if (t <= curve.Keys.front().Time) return curve.Keys.front().Value;
		if (t >= curve.Keys.back().Time)  return curve.Keys.back().Value;

		// locate segment
		auto it = std::upper_bound(curve.Keys.begin(), curve.Keys.end(), t, [](float v, const Component::AnimationCurve::KeyFrame& k)
		{
			return v < k.Time;
		});

		const Component::AnimationCurve::KeyFrame& k1 = *it;           // right key
		const Component::AnimationCurve::KeyFrame& k0 = *(it - 1);     // left  key
		float dt = k1.Time - k0.Time;
		float u = (t - k0.Time) / dt;   // 0 … 1 inside segment

		if (curve.Preset == Component::AnimationCurve::CurvePreset::Linear)
		{
			return glm::mix(k0.Value, k1.Value, u);     // straight
		}

		// cubic Hermite for all other presets
		return Hermite(k0.Value, k0.OutTan * dt, k1.Value, k1.InTan * dt, u);
	}

	void AnimationCurveSystem::SetPreset(Component::AnimationCurve& curve)
	{
		const auto mid = [](Component::AnimationCurve::CurvePreset pr) -> float
		{
			switch (pr)
			{
			case Component::AnimationCurve::CurvePreset::Linear:           return 0.50f;
			case Component::AnimationCurve::CurvePreset::QuadraticEaseIn:  return 0.25f;
			case Component::AnimationCurve::CurvePreset::QuadraticEaseOut: return 0.75f;
			case Component::AnimationCurve::CurvePreset::CubicEaseIn:      return 0.125f;
			case Component::AnimationCurve::CurvePreset::CubicEaseOut:     return 0.875f;
			default:													   return 0.50f;
			}
		};

		curve.Keys = { { 0.0f, 0.0f }, { 0.5f, mid(curve.Preset) }, { 1.0f, 1.0f } };

		RecalculateTangents(curve);
	}

	void AnimationCurveSystem::RecalculateTangents(Component::AnimationCurve& curve)
	{
		ComputeCatmullTangents(curve.Keys);

		switch (curve.Preset)
		{
		case Component::AnimationCurve::CurvePreset::QuadraticEaseIn:
		case Component::AnimationCurve::CurvePreset::CubicEaseIn:
			curve.Keys.front().InTan = curve.Keys.front().OutTan = 0.0f;
			break;
		case Component::AnimationCurve::CurvePreset::QuadraticEaseOut:
		case Component::AnimationCurve::CurvePreset::CubicEaseOut:
			curve.Keys.back().InTan = curve.Keys.back().OutTan = 0.0f;
			break;
		}
	}
}