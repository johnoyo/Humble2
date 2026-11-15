#include "RuntimeSceneSerializer.h"

#include "RuntimeEntitySerializer.h"

namespace HBL2
{
	RuntimeSceneSerializer::RuntimeSceneSerializer(Scene* scene)
		: m_Scene(scene)
	{

	}

	void RuntimeSceneSerializer::Serialize(FileStreamWriter& stream)
	{
		uint64_t indexPos = stream.GetStreamPosition();
		uint32_t entityCount = 0;
		stream.WriteRaw(entityCount);

		m_Scene->Entities()
			.ForEach([&](Entity entity)
			{
				if (entity == Entity::Null)
				{
					return;
				}

				if (m_Scene->GetComponent<Component::Tag>(entity).Name != "Hidden")
				{
					RuntimeEntitySerializer entitySerializer(m_Scene, entity);
					entitySerializer.Serialize(stream);

					entityCount++;
				}
			});

		// TODO: handle custom systems and components.

		// Save the end position to reset after patch.
		uint64_t endPos = stream.GetStreamPosition();

		// Reset cursor position.
		stream.SetStreamPosition(indexPos);

		// Write component count.
		stream.WriteRaw<uint32_t>(entityCount);

		// Set again the cursor at the end position.
		stream.SetStreamPosition(endPos);
	}

	bool RuntimeSceneSerializer::Deserialize(FileStreamReader& stream)
	{
		uint32_t entityCount;
		stream.ReadRaw(entityCount);

		for (uint32_t i = 0; i < entityCount; i++)
		{
			RuntimeEntitySerializer entitySerializer(m_Scene);
			entitySerializer.Deserialize(stream);
		}

		// TODO: handle custom systems and components.
		
		// TODO: handle prefabs?

		return false;
	}
}
