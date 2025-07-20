#pragma once

#include "Humble2Core.h"

using namespace HBL2;

class ItemSystem final : public HBL2::ISystem
{
public:
	virtual void OnCreate() override
	{
		m_Context->View<ItemManager>()
			.Each([&](ItemManager& itemManager)
			{
				AssetManager::Instance->GetAsset<Prefab>(itemManager.PowerUpPrefab);
				AssetManager::Instance->GetAsset<Prefab>(itemManager.ObstaclePrefab);
				AssetManager::Instance->GetAsset<Prefab>(itemManager.CoinPrefab);
			});

		m_PowerUpSpawnTimer.Reset();
		m_ObstacleSpawnTimer.Reset();
		m_CoinSpawnTimer.Reset();
	}

	virtual void OnUpdate(float ts) override
	{
		m_Context->View<ItemManager>()
			.Each([&](ItemManager& itemManager)
			{
				if (!itemManager.Enabled)
				{
					return;
				}

				if (itemManager.Reset)
				{
					m_PowerUpSpawnTimer.Reset();
					m_ObstacleSpawnTimer.Reset();
					m_CoinSpawnTimer.Reset();

					m_SpeedsterCrashed = false;
					itemManager.Reset = false;
				}

				if (m_PowerUpSpawnTimer.Elapsed() > 6.f && !m_SpeedsterCrashed)
				{
					HBL2_INFO("Spawning powerup...");

					float spawnOffset = Random::Float(-3.1f, 3.1f);

					Handle<Asset> powerUpPrefab = AssetManager::Instance->GetHandleFromUUID(itemManager.PowerUpPrefab);
					Prefab::Instantiate(powerUpPrefab, { 20.678f + spawnOffset, 0.2f, -57.075f });

					m_PowerUpSpawnTimer.Reset();
				}

				if (m_ObstacleSpawnTimer.Elapsed() > 2.5f && !m_SpeedsterCrashed)
				{
					HBL2_INFO("Spawning obstacle...");

					float spawnOffset = Random::Float(-3.3f, 3.3f);

					Handle<Asset> obstaclePrefab = AssetManager::Instance->GetHandleFromUUID(itemManager.ObstaclePrefab);
					Prefab::Instantiate(obstaclePrefab, { 15.527f + spawnOffset, 0.2f, -48.493f });

					m_ObstacleSpawnTimer.Reset();
				}

				if (m_CoinSpawnTimer.Elapsed() > 8.f && !m_SpeedsterCrashed)
				{
					HBL2_INFO("Spawning coin...");

					float spawnOffset = Random::Float(-3.3f, 3.3f);

					Handle<Asset> coinPrefab = AssetManager::Instance->GetHandleFromUUID(itemManager.CoinPrefab);
					Prefab::Instantiate(coinPrefab, { 28.886f + spawnOffset, 0.2f, -69.934f });

					m_CoinSpawnTimer.Reset();
				}
			});
	}

	virtual void OnFixedUpdate() override
	{
		float speed = 7.f;

		// Check if the car has crashed.
		m_Context->View<Speedster>()
			.Each([&](Speedster& speedster)
			{
				speed = speedster.Speed;

				if (!speedster.Enabled)
				{
					m_SpeedsterCrashed = true;
					return;
				}

				m_SpeedsterCrashed = false;
			});

		DynamicArray<Entity, BumpAllocator> itemsToDestroy = MakeDynamicArray<Entity>(&Allocator::Frame);

		m_Context->View<Item, Component::Transform, Component::Rigidbody>()
			.Each([&](Entity e, Item& item, Component::Transform& tr, Component::Rigidbody& rb)
			{
				// If the car has crashed disable the track chunks.
				if (m_SpeedsterCrashed || !item.Enabled)
				{
					item.Enabled = false;
					PhysicsEngine3D::Instance->SetAngularVelocity(rb, { 0.f, 0.f, 0.f });
					PhysicsEngine3D::Instance->SetLinearVelocity(rb, { 0.f, 0.f, 0.f });
					return;
				}

				if (tr.Translation.x >= -55.f && tr.Translation.z <= 85.f)
				{
					float angleYRadians = glm::radians(tr.Rotation.y);

					float deltaX = sin(angleYRadians) * speed;
					float deltaZ = cos(angleYRadians) * speed;

					PhysicsEngine3D::Instance->SetLinearVelocity(rb, { deltaX, 0.f, deltaZ });
				}
				else
				{
					itemsToDestroy.Add(e);
				}
			});

		for (Entity e : itemsToDestroy)
		{
			Prefab::Destroy(e);
		}
	}

private:
	bool m_SpeedsterCrashed = false;
	Timer m_PowerUpSpawnTimer;
	Timer m_ObstacleSpawnTimer;
	Timer m_CoinSpawnTimer;
};

REGISTER_HBL2_SYSTEM(ItemSystem)
