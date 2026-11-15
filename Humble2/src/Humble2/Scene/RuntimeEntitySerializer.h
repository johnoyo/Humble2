#pragma once

#include "Scene.h"
#include "Serialization/FileStreamWriter.h"
#include "Serialization/FileStreamReader.h"

namespace HBL2
{
	class RuntimeEntitySerializer
	{
	public:
		RuntimeEntitySerializer(Scene* scene);
		RuntimeEntitySerializer(Scene* scene, Entity entity);

		void Serialize(FileStreamWriter& stream);
		bool Deserialize(FileStreamReader& stream);

	private:
		Scene* m_Scene = nullptr;
		Entity m_Entity = Entity::Null;
	};
}
