#include "HierachySystem.h"

namespace HBL2
{
	void HierachySystem::OnCreate()
	{
		m_Context->View<Component::Transform>()
			.Each([&](Component::Transform& transform)
			{
				glm::mat4 T = glm::translate(glm::mat4(1.0f), transform.Translation);
				transform.QRotation = glm::quat({ glm::radians(transform.Rotation.x), glm::radians(transform.Rotation.y), glm::radians(transform.Rotation.z) });
				glm::mat4 S = glm::scale(glm::mat4(1.0f), transform.Scale);

				transform.LocalMatrix = T * glm::toMat4(transform.QRotation) * S;
				transform.WorldMatrix = transform.LocalMatrix;
			});

		m_Context->Group<Component::Link>(Get<Component::Transform>)
			.Each([&](Entity entity, Component::Link& link, Component::Transform& transform)
			{
				link.Children.clear();
			});

		m_Context->Group<Component::Link>(Get<Component::Transform>)
			.Each([&](Entity entity, Component::Link& link, Component::Transform& transform)
			{
				transform.WorldMatrix = GetWorldSpaceTransform(entity, link);
				AddChildren(entity, link);
			});
	}

	void HierachySystem::OnUpdate(float ts)
	{
		BEGIN_PROFILE_SYSTEM();

		// Handle world translation changes.
		m_Context->Group<Component::Link>(Get<Component::Transform>)
			.Each([&](Entity entity, Component::Link& link, Component::Transform& transform)
			{
				if (transform.PrevWorldTranslation == transform.WorldTranslation)
				{
					return;
				}

				// Fetch parent world matrix
				glm::mat4 parentW = glm::mat4(1.0f);
				if (link.Parent != 0)
				{
					Entity p = m_Context->FindEntityByUUID(link.Parent);
					if (p != Entity::Null)
					{
						parentW = m_Context->GetComponent<Component::Transform>(p).WorldMatrix;
					}
				}

				// Compute new local translation = inverse(parent) * desired world
				glm::vec3 localPos = glm::vec3(glm::inverse(parentW) * glm::vec4(transform.WorldTranslation, 1));

				// Set local translation
				if (localPos != transform.Translation)
				{
					transform.Translation = localPos;
				}
			});

		// Calculate local matrices.
		m_Context->View<Component::Transform>()
			.Each([&](Component::Transform& transform)
			{
				if (!transform.Static)
				{
					glm::mat4 T = glm::translate(glm::mat4(1.0f), transform.Translation);
					transform.QRotation = glm::quat({ glm::radians(transform.Rotation.x), glm::radians(transform.Rotation.y), glm::radians(transform.Rotation.z) });
					glm::mat4 S = glm::scale(glm::mat4(1.0f), transform.Scale);

					glm::mat4 newLocalMatrix = T * glm::toMat4(transform.QRotation) * S;

					if (transform.LocalMatrix != newLocalMatrix)
					{
						transform.LocalMatrix = std::move(newLocalMatrix);
						transform.WorldMatrix = transform.LocalMatrix;
						transform.Dirty = true;
					}
					else
					{
						transform.Dirty = false;
					}
				}
			});

		// Calculate world matrices.
		m_Context->Group<Component::Link>(Get<Component::Transform>)
			.Each([&](Entity entity, Component::Link& link, Component::Transform& transform)
			{
				if (!transform.Static)
				{
					if (transform.Dirty)
					{
						transform.WorldMatrix = GetWorldSpaceTransform(entity, link);
						transform.WorldTranslation = glm::vec3(transform.WorldMatrix[3]);
						transform.Dirty = false;
					}

					UpdateChildren(entity, link);
				}
			});

		END_PROFILE_SYSTEM(RunningTime);
	}

	glm::mat4 HierachySystem::GetWorldSpaceTransform(Entity entity, Component::Link& link)
	{
		glm::mat4 transform = glm::mat4(1.0f);

		if (link.Parent != 0)
		{
			Entity parentEntity = m_Context->FindEntityByUUID(link.Parent);
			if (parentEntity != Entity::Null)
			{
				Component::Link& parentLink = m_Context->GetComponent<Component::Link>(parentEntity);
				transform = GetWorldSpaceTransform(parentEntity, parentLink);
			}
		}

		return transform * m_Context->GetComponent<Component::Transform>(entity).LocalMatrix;
	}

	void HierachySystem::AddChildren(Entity entity, Component::Link& link)
	{
		// Add the entity to its new parent's children list, if it has one.
		if (link.Parent != 0)
		{
			Entity parent = m_Context->FindEntityByUUID(link.Parent);
			auto* parentLink = m_Context->TryGetComponent<Component::Link>(parent);

			if (parentLink)
			{
				parentLink->Children.push_back(m_Context->GetComponent<Component::ID>(entity).Identifier);
			}
		}
	}

	void HierachySystem::UpdateChildren(Entity entity, Component::Link& link)
	{
		// Check if the entity's parent has changed or if it's no longer a child.
		if (link.PrevParent == link.Parent)
		{
			return;
		}

		// Remove the entity from its previous parent's children list, if it had one.
		if (link.PrevParent != 0)
		{
			Entity prevParent = m_Context->FindEntityByUUID(link.PrevParent);
			auto* prevParentLink = m_Context->TryGetComponent<Component::Link>(prevParent);

			if (prevParentLink)
			{
				UUID childUUID = m_Context->GetComponent<Component::ID>(entity).Identifier;
				auto childrenIterator = std::find(prevParentLink->Children.begin(), prevParentLink->Children.end(), childUUID);

				if (childrenIterator != prevParentLink->Children.end())
				{
					prevParentLink->Children.erase(childrenIterator);
				}
			}
		}

		// Add the entity to its new parent's children list, if it has one.
		if (link.Parent != 0)
		{
			Entity parent = m_Context->FindEntityByUUID(link.Parent);
			auto* parentLink = m_Context->TryGetComponent<Component::Link>(parent);

			if (parentLink)
			{
				UUID childUUID = m_Context->GetComponent<Component::ID>(entity).Identifier;
				parentLink->Children.push_back(childUUID);
			}
		}

		// Update previous parent to be the current
		link.PrevParent = link.Parent;
	}
}
