#include "HierachySystem.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace HBL2
{
	static bool DecomposeTRS(const glm::mat4& m, glm::vec3& outTranslation, glm::vec3& outRotationDeg, glm::vec3& outScale )
	{
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::quat orientation;

		if (!glm::decompose(m, outScale, orientation, outTranslation, skew, perspective))
		{
			return false;
		}

		// Convert quat -> euler radians -> degrees
		outRotationDeg = glm::degrees(glm::eulerAngles(orientation));
		return true;
	}

	static glm::mat4 TRS(const glm::vec3& t, const glm::vec3& rDeg, const glm::vec3& s)
	{
		glm::mat4 T = glm::translate(glm::mat4(1.0f), t);
		glm::quat q = glm::quat(glm::radians(rDeg));
		glm::mat4 R = glm::toMat4(q);
		glm::mat4 S = glm::scale(glm::mat4(1.0f), s);
		return T * R * S;
	}

	void HierachySystem::OnCreate()
	{
		m_Context->View<Component::Transform>()
			.Each([&](Component::Transform& transform)
			{
				glm::mat4 T = glm::translate(glm::mat4(1.0f), transform.Translation);
				transform.QRotation = glm::quat(glm::radians(transform.Rotation));
				glm::mat4 S = glm::scale(glm::mat4(1.0f), transform.Scale);

				transform.LocalMatrix = T * glm::toMat4(transform.QRotation) * S;
				transform.WorldMatrix = transform.LocalMatrix;
			});

		m_Context->Group<Component::Transform, Component::Link>()
			.Each([&](Entity entity, Component::Transform& transform, Component::Link& link)
			{
				link.Children.clear();
			});

		m_Context->Group<Component::Transform, Component::Link>()
			.Each([&](Entity entity, Component::Transform& transform, Component::Link& link)
			{
				transform.WorldMatrix = GetWorldSpaceTransform(entity, link);
				AddChildren(entity, link);
			});
	}

	void HierachySystem::OnUpdate(float ts)
	{
		BEGIN_PROFILE_SYSTEM();

		m_Context->Group<Component::Transform, Component::Link>()
			.Each([&](Entity entity, Component::Transform& transform, Component::Link& link)
			{
				const bool worldTChanged = (transform.PrevWorldTranslation != transform.WorldTranslation);
				const bool worldRChanged = (transform.PrevWorldRotation != transform.WorldRotation);
				const bool worldSChanged = (transform.PrevWorldScale != transform.WorldScale);

				// Handle world transform changes.
				if (!transform.Static && (worldTChanged || worldRChanged || worldSChanged))
				{
					// Fetch parent world matrix.
					glm::mat4 parentW(1.0f);
					if (link.Parent != 0)
					{
						Entity p = m_Context->FindEntityByUUID(link.Parent);
						if (p != Entity::Null)
						{
							parentW = m_Context->GetComponent<Component::Transform>(p).WorldMatrix;
						}
					}

					glm::mat4 desiredWorld = TRS(transform.WorldTranslation, transform.WorldRotation, transform.WorldScale);
					glm::mat4 localM = glm::inverse(parentW) * desiredWorld;

					// Set local transforms.
					glm::vec3 lt, lr, ls;
					if (DecomposeTRS(localM, lt, lr, ls))
					{
						transform.Translation = lt;
						transform.Rotation = lr;
						transform.Scale = ls;
					}

					transform.PrevWorldTranslation = transform.WorldTranslation;
					transform.PrevWorldRotation = transform.WorldRotation;
					transform.PrevWorldScale = transform.WorldScale;
				}

				// Calculate local matrices.
				if (!transform.Static)
				{
					transform.QRotation = glm::quat(glm::radians(transform.Rotation));
					transform.LocalMatrix = TRS(transform.Translation, transform.Rotation, transform.Scale);
				}
			});

		// Calculate world matrices.
		m_Context->Group<Component::Transform, Component::Link>()
			.Each([&](Entity entity, Component::Transform& transform, Component::Link& link)
			{
				if (!transform.Static)
				{
					transform.WorldMatrix = GetWorldSpaceTransform2(entity, link);

					// Set world transforms.
					glm::vec3 wt, wr, ws;
					if (DecomposeTRS(transform.WorldMatrix, wt, wr, ws))
					{
						transform.WorldTranslation = wt;
						transform.WorldRotation = wr;
						transform.WorldScale = ws;
					}
					else
					{
						transform.WorldTranslation = glm::vec3(transform.WorldMatrix[3]);
					}

					transform.PrevWorldTranslation = transform.WorldTranslation;
					transform.PrevWorldRotation = transform.WorldRotation;
					transform.PrevWorldScale = transform.WorldScale;

					UpdateChildren(entity, link);
				}
			});

		END_PROFILE_SYSTEM(RunningTime);
	}

	glm::mat4 HierachySystem::GetWorldSpaceTransform(Entity entity, Component::Link& link)
	{
		const UUID id = m_Context->GetComponent<Component::ID>(entity).Identifier;

		// cache hit
		if (auto it = m_WorldCache.find(id); it != m_WorldCache.end())
		{
			return it->second;
		}

		glm::mat4 parentW(1.0f);

		if (link.Parent != 0)
		{
			Entity parentEntity = m_Context->FindEntityByUUID(link.Parent);
			if (parentEntity != Entity::Null)
			{
				auto& parentLink = m_Context->GetComponent<Component::Link>(parentEntity);
				parentW = GetWorldSpaceTransform(parentEntity, parentLink);
			}
		}

		glm::mat4 world = parentW * m_Context->GetComponent<Component::Transform>(entity).LocalMatrix;
		m_WorldCache.emplace(id, world);
		return world;
	}


	glm::mat4 HierachySystem::GetWorldSpaceTransform2(Entity entity, Component::Link& link)
	{
		glm::mat4 transform = glm::mat4(1.0f);

		if (link.Parent != 0)
		{
			Entity parentEntity = m_Context->FindEntityByUUID(link.Parent);
			if (parentEntity != Entity::Null)
			{
				Component::Link& parentLink = m_Context->GetComponent<Component::Link>(parentEntity);
				transform = GetWorldSpaceTransform2(parentEntity, parentLink);
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
