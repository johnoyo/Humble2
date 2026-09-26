#pragma once

#include "Humble2Core.h"

class BenchmarkSystem final : public HBL2::ISystem
{
public:
	virtual void OnCreate() override
	{
		m_Context->GetRegistry()
			.Filter<BenchmarkComponent>()
			.ForEach([this](BenchmarkComponent& benchmark)
			{
				CONSOLE_LOG("Cubes: {0}", benchmark.Cubes);

				if (benchmark.Terrain)
				{
					HBL2::Entity terrainEntity = m_Context->CreateEntity("Terrain");

					m_Context->AddComponent<HBL2::Component::EditorVisible>(terrainEntity);
					auto& curve = m_Context->AddComponent<HBL2::Component::AnimationCurve>(terrainEntity);
					curve.Preset = HBL2::Component::AnimationCurve::CurvePreset::QuadraticEaseIn;

					auto& terrain = m_Context->AddComponent<HBL2::Component::Terrain>(terrainEntity);
					terrain.Material = benchmark.TerrainMaterial;
					terrain.DetailLevels.push_back({ .Lod = 0, .VisibleDstThreshold = 200 });
					terrain.DetailLevels.push_back({ .Lod = 2, .VisibleDstThreshold = 400 });
					terrain.DetailLevels.push_back({ .Lod = 5, .VisibleDstThreshold = 600 });
					terrain.HeightMultiplier = 17.f;
					terrain.AddColliders = benchmark.EnablePhysics;
				}

				// PhysicsCubes
				auto physicsCubes = m_Context->CreateEntity("PhysicsCubes");
				auto& physicsCubesID = m_Context->GetComponent<HBL2::Component::ID>(physicsCubes);
                
                float spacing = 2.5f;
                
				int32_t numPhysicsCubesPerEdge = (int32_t)glm::ceil(glm::sqrt(benchmark.PhysicsCubes));
				for (int32_t i = -numPhysicsCubesPerEdge / 2; i < numPhysicsCubesPerEdge / 2; i++)
				{
					for (int32_t j = -numPhysicsCubesPerEdge / 2; j < numPhysicsCubesPerEdge / 2; j++)
					{
						auto entity = m_Context->CreateEntity("PhysicsCube");

						auto& link = m_Context->GetComponent<HBL2::Component::Link>(entity);
						link.Parent = physicsCubesID.Identifier;

						auto& transform = m_Context->GetComponent<HBL2::Component::Transform>(entity);
						transform.Translation.x = i * spacing;
						transform.Translation.y = 120.f;
						transform.Translation.z = -j * spacing;

						m_Context->AddComponent<HBL2::Component::EditorVisible>(entity);

						auto& staticMesh = m_Context->AddComponent<HBL2::Component::StaticMesh>(entity);
						staticMesh.Mesh = HBL2::MeshUtilities::Get().GetBuiltInLoadedMeshAssetHandle(HBL2::BuiltInMesh::CUBE);
						staticMesh.Material = benchmark.PhysicsCubeMaterial;

						if (benchmark.EnablePhysics)
						{
							auto& rb = m_Context->AddComponent<HBL2::Component::Rigidbody>(entity);
							rb.Type = HBL2::Physics::BodyType::Dynamic;

							auto& bc = m_Context->AddComponent<HBL2::Component::BoxCollider>(entity);
						}
                    }
				}
                
                // Cubes
                auto cubes = m_Context->CreateEntity("Cubes");
                auto& cubesID = m_Context->GetComponent<HBL2::Component::ID>(cubes);

                int32_t numCubesPerEdge = (int32_t)glm::ceil(glm::sqrt(benchmark.Cubes));
                for (int32_t i = -numCubesPerEdge / 2; i < numCubesPerEdge / 2; i++)
                {
                    for (int32_t j = -numCubesPerEdge / 2; j < numCubesPerEdge / 2; j++)
                    {
                        auto entity = m_Context->CreateEntity("Cube");

                        auto& link = m_Context->GetComponent<HBL2::Component::Link>(entity);
                        link.Parent = cubesID.Identifier;

                        auto& transform = m_Context->GetComponent<HBL2::Component::Transform>(entity);
                        transform.Translation.x = i;
                        transform.Translation.y = 20.f;
                        transform.Translation.z = -j;

                        m_Context->AddComponent<HBL2::Component::EditorVisible>(entity);

                        auto& staticMesh = m_Context->AddComponent<HBL2::Component::StaticMesh>(entity);
                        staticMesh.Mesh = HBL2::MeshUtilities::Get().GetBuiltInLoadedMeshAssetHandle(HBL2::BuiltInMesh::CUBE);
                        staticMesh.Material = benchmark.CubeMaterial;
                    }
                }

				// Spheres
				auto spheres = m_Context->CreateEntity("Spheres");
				auto& spheresID = m_Context->GetComponent<HBL2::Component::ID>(spheres);

				int32_t numSpheresPerEdge = (int32_t)glm::ceil(glm::sqrt(benchmark.Spheres));
				for (int32_t i = -numSpheresPerEdge / 2; i < numSpheresPerEdge / 2; i++)
				{
					for (int32_t j = -numSpheresPerEdge / 2; j < numSpheresPerEdge / 2; j++)
					{
						auto entity = m_Context->CreateEntity("Sphere");

						auto& link = m_Context->GetComponent<HBL2::Component::Link>(entity);
						link.Parent = spheresID.Identifier;

						auto& transform = m_Context->GetComponent<HBL2::Component::Transform>(entity);
						transform.Translation.x = i;
						transform.Translation.y = 30.f;
						transform.Translation.z = -j;

						m_Context->AddComponent<HBL2::Component::EditorVisible>(entity);

						auto& staticMesh = m_Context->AddComponent<HBL2::Component::StaticMesh>(entity);
						staticMesh.Mesh = HBL2::MeshUtilities::Get().GetBuiltInLoadedMeshAssetHandle(HBL2::BuiltInMesh::SPHERE);
						staticMesh.Material = benchmark.SphereMaterial;
					}
				}

				// Cylinders
				auto cylinders = m_Context->CreateEntity("Cylinders");
				auto& cylindersID = m_Context->GetComponent<HBL2::Component::ID>(cylinders);

				int32_t numCylindersPerEdge = (int32_t)glm::ceil(glm::sqrt(benchmark.Cylinders));
				for (int32_t i = -numCylindersPerEdge / 2; i < numCylindersPerEdge / 2; i++)
				{
					for (int32_t j = -numCylindersPerEdge / 2; j < numCylindersPerEdge / 2; j++)
					{
						auto entity = m_Context->CreateEntity("Cylinder");

						auto& link = m_Context->GetComponent<HBL2::Component::Link>(entity);
						link.Parent = cylindersID.Identifier;

						auto& transform = m_Context->GetComponent<HBL2::Component::Transform>(entity);
						transform.Translation.x = i;
						transform.Translation.y = 40.f;
						transform.Translation.z = -j;

						m_Context->AddComponent<HBL2::Component::EditorVisible>(entity);

						auto& staticMesh = m_Context->AddComponent<HBL2::Component::StaticMesh>(entity);
						staticMesh.Mesh = HBL2::MeshUtilities::Get().GetBuiltInLoadedMeshAssetHandle(HBL2::BuiltInMesh::CYLINDER);
						staticMesh.Material = benchmark.CylinderMaterial;
					}
				}

				// Torus
				auto toruses = m_Context->CreateEntity("Toruses");
				auto& torusesID = m_Context->GetComponent<HBL2::Component::ID>(toruses);

				int32_t numTorusesPerEdge = (int32_t)glm::ceil(glm::sqrt(benchmark.Torus));
				for (int32_t i = -numTorusesPerEdge / 2; i < numTorusesPerEdge / 2; i++)
				{
					for (int32_t j = -numTorusesPerEdge / 2; j < numTorusesPerEdge / 2; j++)
					{
						auto entity = m_Context->CreateEntity("Torus");

						auto& link = m_Context->GetComponent<HBL2::Component::Link>(entity);
						link.Parent = torusesID.Identifier;

						auto& transform = m_Context->GetComponent<HBL2::Component::Transform>(entity);
						transform.Translation.x = i;
						transform.Translation.y = 50.f;
						transform.Translation.z = -j;

						m_Context->AddComponent<HBL2::Component::EditorVisible>(entity);

						auto& staticMesh = m_Context->AddComponent<HBL2::Component::StaticMesh>(entity);
						staticMesh.Mesh = HBL2::MeshUtilities::Get().GetBuiltInLoadedMeshAssetHandle(HBL2::BuiltInMesh::TORUS);
						staticMesh.Material = benchmark.TorusMaterial;
					}
				}

				// Capsules
				auto capsules = m_Context->CreateEntity("Capsules");
				auto& capsulesID = m_Context->GetComponent<HBL2::Component::ID>(capsules);

				int32_t numCapsulesPerEdge = (int32_t)glm::ceil(glm::sqrt(benchmark.Torus));
				for (int32_t i = -numCapsulesPerEdge / 2; i < numCapsulesPerEdge / 2; i++)
				{
					for (int32_t j = -numCapsulesPerEdge / 2; j < numCapsulesPerEdge / 2; j++)
					{
						auto entity = m_Context->CreateEntity("Capsule");

						auto& link = m_Context->GetComponent<HBL2::Component::Link>(entity);
						link.Parent = capsulesID.Identifier;

						auto& transform = m_Context->GetComponent<HBL2::Component::Transform>(entity);
						transform.Translation.x = i;
						transform.Translation.y = 60.f;
						transform.Translation.z = -j;

						m_Context->AddComponent<HBL2::Component::EditorVisible>(entity);

						auto& staticMesh = m_Context->AddComponent<HBL2::Component::StaticMesh>(entity);
						staticMesh.Mesh = HBL2::MeshUtilities::Get().GetBuiltInLoadedMeshAssetHandle(HBL2::BuiltInMesh::CAPSULE);
						staticMesh.Material = benchmark.CapsuleMaterial;
					}
				}
			});
	}

	virtual void OnUpdate(float ts) override
	{
	}
};

REGISTER_HBL2_SYSTEM(BenchmarkSystem)
