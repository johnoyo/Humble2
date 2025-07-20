#pragma once

#include "Humble2Core.h"

#include <glm/gtx/quaternion.hpp>

using namespace HBL2;

class TrackChunkSystem final : public HBL2::ISystem
{
public:
	virtual void OnCreate() override
	{
		m_Context->View<TrackChunk, Component::Transform>()
			.Each([&](TrackChunk& trackChunk, Component::Transform& tr)
			{
				trackChunk.SpawnPosition = tr.Translation;
				trackChunk.InitialPosition = { 34.690f, 0.0f, -70.940f };
				trackChunk.PositionToReset = { -55.0f, 0.0f, 85.0f };
			});
	}

	virtual void OnUpdate(float ts) override
	{
		float speed = 5.f;
		bool speedsterCrashed = false;

		// Check if the car has crashed.
		m_Context->View<Speedster>()
			.Each([&](Speedster& speedster)
			{
				speed = speedster.Speed;

				if (!speedster.Enabled)
				{
					speedsterCrashed = true;
					return;
				}
			});

		m_Context->View<TrackChunk, Component::Transform>()
			.Each([&](TrackChunk& trackChunk, Component::Transform& tr)
			{
				// If the car has crashed disable the track chunks.
				if (speedsterCrashed || !trackChunk.Enabled)
				{
					trackChunk.Enabled = false;
					return;
				}

				if (tr.Translation.x >= trackChunk.PositionToReset.x && tr.Translation.z <= trackChunk.PositionToReset.z)
				{
					float angleYRadians = glm::radians(tr.Rotation.y);

					float deltaX = sin(angleYRadians) * speed;
					float deltaZ = cos(angleYRadians) * speed;

					glm::vec3 movement(deltaX, 0.0f, deltaZ);

					tr.Translation += movement * ts;
				}
				else
				{
					tr.Translation = trackChunk.InitialPosition;
					speed += 0.5f;
				}
			});

		// Set the updated speed back to the speedster.
		m_Context->View<Speedster>()
			.Each([&](Speedster& speedster)
			{
				speedster.Speed = speed;
			});
	}
};

REGISTER_HBL2_SYSTEM(TrackChunkSystem)
