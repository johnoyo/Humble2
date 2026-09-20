#pragma once

#include "Humble2Core.h"

// Just a POD struct
struct BenchmarkComponent
{
    using storage_type = HBL2::SingletonComponentStorage<BenchmarkComponent>;

    int Cubes = 10000;
    HBL2::Handle<HBL2::Asset> CubeMaterial;

    int Spheres = 10000;
    HBL2::Handle<HBL2::Asset> SphereMaterial;

    int Cylinders = 10000;
    HBL2::Handle<HBL2::Asset> CylinderMaterial;

    int Torus = 10000;
    HBL2::Handle<HBL2::Asset> TorusMaterial;

    int Capsules = 10000;
    HBL2::Handle<HBL2::Asset> CapsuleMaterial;

    int Monkeys = 10000;
    int Sprites = 10000;
    bool UniqueMaterials = true;

    bool Terrain = true;
    HBL2::Handle<HBL2::Asset> TerrainMaterial;

	// Member registration.
	static constexpr auto schema = HBL2::Reflect::Schema
	{
        HBL2::Reflect::Field{"Cubes", &BenchmarkComponent::Cubes},
        HBL2::Reflect::Field{"CubeMaterial", &BenchmarkComponent::CubeMaterial},
        HBL2::Reflect::Field{"Spheres", &BenchmarkComponent::Spheres},
        HBL2::Reflect::Field{"SphereMaterial", &BenchmarkComponent::SphereMaterial},
        HBL2::Reflect::Field{"Cylinders", &BenchmarkComponent::Cylinders},
        HBL2::Reflect::Field{"CylinderMaterial", &BenchmarkComponent::CylinderMaterial},
        HBL2::Reflect::Field{"Torus", &BenchmarkComponent::Torus},
        HBL2::Reflect::Field{"TorusMaterial", &BenchmarkComponent::TorusMaterial},
        HBL2::Reflect::Field{"Capsules", &BenchmarkComponent::Capsules},
        HBL2::Reflect::Field{"CapsuleMaterial", &BenchmarkComponent::CapsuleMaterial},
        HBL2::Reflect::Field{"Monkeys", &BenchmarkComponent::Monkeys},
        HBL2::Reflect::Field{"Sprites", &BenchmarkComponent::Sprites},
        HBL2::Reflect::Field{"UniqueMaterials", &BenchmarkComponent::UniqueMaterials},
        HBL2::Reflect::Field{"Terrain", &BenchmarkComponent::Terrain},
        HBL2::Reflect::Field{"TerrainMaterial", &BenchmarkComponent::TerrainMaterial},
    };
};

// Register component
REGISTER_HBL2_COMPONENT(BenchmarkComponent)
