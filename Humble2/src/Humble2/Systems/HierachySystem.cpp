#include "HierachySystem.h"

#include "Core\Allocators.h"
#include "Utilities\Collections\Collections.h"

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
		m_Context->Group<Component::Transform, Component::TransformEx, Component::Link>()
			.Each([&](Entity entity, Component::Transform& transform, Component::TransformEx& transformEx, Component::Link& link)
			{
				// Calculate initial matrices.
				glm::mat4 T = glm::translate(glm::mat4(1.0f), transform.Translation);
				transform.QRotation = glm::quat(glm::radians(transform.Rotation));
				glm::mat4 S = glm::scale(glm::mat4(1.0f), transform.Scale);

				transform.LocalMatrix = T * glm::toMat4(transform.QRotation) * S;
				transform.WorldMatrix = transform.LocalMatrix;

				// Clear the list of children.
				link.Children.clear();
			});

		m_Context->Group<Component::Transform, Component::TransformEx, Component::Link>()
			.Each([&](Entity entity, Component::Transform& transform, Component::TransformEx& transformEx, Component::Link& link)
			{
				transform.WorldMatrix = GetWorldSpaceTransform(entity, link);
				AddChildren(entity, link);
			});
	}

	void HierachySystem::OnUpdate(float ts)
	{
		BEGIN_PROFILE_SYSTEM();

		// Handle world edits (world TRS -> local TRS).
		m_Context->Group<Component::Transform, Component::TransformEx, Component::Link>()
			.Each([&](Entity entity, Component::Transform& transform, Component::TransformEx& transformEx, Component::Link& link)
			{
				if (transform.Static)
				{
					return;
				}

				const bool worldTChanged = (transformEx.PrevWorldTranslation != transform.WorldTranslation);
				const bool worldRChanged = (transformEx.PrevWorldRotation != transform.WorldRotation);
				const bool worldSChanged = (transformEx.PrevWorldScale != transform.WorldScale);

				if (worldTChanged || worldRChanged || worldSChanged)
				{
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

					glm::vec3 lt, lr, ls;
					if (DecomposeTRS(localM, lt, lr, ls))
					{
						transform.Translation = lt;
						transform.Rotation = lr;
						transform.Scale = ls;
					}

					// local changed -> mark dirty
					MarkDirtyRecursive(entity);

					transformEx.PrevWorldTranslation = transform.WorldTranslation;
					transformEx.PrevWorldRotation = transform.WorldRotation;
					transformEx.PrevWorldScale = transform.WorldScale;
				}
			});

		// Update children lists only when parent changes.
		m_Context->Group<Component::Transform, Component::TransformEx, Component::Link>()
			.Each([&](Entity entity, Component::Transform& transform, Component::TransformEx& transformEx, Component::Link& link)
			{
				// this already early-outs if no change
				const UUID before = link.PrevParent;
				UpdateChildren(entity, link);

				if (before != link.PrevParent) // parent changed
				{
					MarkDirtyRecursive(entity);
				}
			});

		// Rebuild local matrices only for dirty nodes.
		m_Context->Group<Component::Transform, Component::TransformEx, Component::Link>()
			.Each([&](Entity entity, Component::Transform& transform, Component::TransformEx& transformEx, Component::Link& link)
			{
				if (transform.Static)
				{
					return;
				}

				const bool localTChanged = (transformEx.PrevTranslation != transform.Translation);
				const bool localRChanged = (transformEx.PrevRotation != transform.Rotation);
				const bool localSChanged = (transformEx.PrevScale != transform.Scale);

				if (localTChanged || localRChanged || localSChanged)
				{
					MarkDirtyRecursive(entity);
					transformEx.PrevTranslation = transform.Translation;
					transformEx.PrevRotation = transform.Rotation;
					transformEx.PrevScale = transform.Scale;
				}

				if (!transform.Dirty)
				{
					return;
				}

				transform.QRotation = glm::quat(glm::radians(transform.Rotation));
				transform.LocalMatrix = TRS(transform.Translation, transform.Rotation, transform.Scale);
			});

		// World matrices: single root traversal.
		ComputeWorldFromDirtyRoots();

		// Update world TRS.
		m_Context->Group<Component::Transform, Component::TransformEx, Component::Link>()
			.Each([&](Entity entity, Component::Transform& transform, Component::TransformEx& transformEx, Component::Link& link)
				{
					if (transform.Static || !transform.Dirty)
					{
						return;
					}

					// Always cheap position
					transform.WorldTranslation = glm::vec3(transform.WorldMatrix[3]);

					// Decompose rotation/scale in world space (maybe add an ifdef to happen only in editor).
					glm::vec3 wt, wr, ws;
					if (DecomposeTRS(transform.WorldMatrix, wt, wr, ws))
					{
						transform.WorldRotation = wr;
						transform.WorldScale = ws;
					}

					transformEx.PrevWorldTranslation = transform.WorldTranslation;
					transformEx.PrevWorldRotation = transform.WorldRotation;
					transformEx.PrevWorldScale = transform.WorldScale;

					transform.Dirty = false; // done
				});

		END_PROFILE_SYSTEM(RunningTime);
	}
	
	void HierachySystem::ComputeWorldFromDirtyRoots()
	{
		DArray<Entity> dirtyRoots = MakeDArray<Entity>(Allocator::FrameArena, 256);

		// Find "dirty roots" (dirty nodes whose parent is not dirty, or no/invalid parent)
		m_Context->Group<Component::Transform, Component::TransformEx, Component::Link>()
			.Each([&](Entity e, Component::Transform& t, Component::TransformEx& tEx, Component::Link& l)
			{
				if (t.Static || !t.Dirty)
				{
					return;
				}

				if (l.Parent == 0)
				{
					dirtyRoots.push_back(e);
					return;
				}

				Entity p = m_Context->FindEntityByUUID(l.Parent);
				if (p == Entity::Null)
				{
					dirtyRoots.push_back(e);
					return;
				}

				auto* pt = m_Context->TryGetComponent<Component::Transform>(p);
				if (!pt || pt->Static || !pt->Dirty)
				{
					dirtyRoots.push_back(e);
				}
			});

		// Traverse only dirty subtrees.
		DArray<Entity> stack = MakeDArray<Entity>(Allocator::FrameArena, 512);

		for (Entity root : dirtyRoots)
		{
			auto& rt = m_Context->GetComponent<Component::Transform>(root);
			auto& rl = m_Context->GetComponent<Component::Link>(root);

			glm::mat4 parentW(1.0f);
			if (rl.Parent != 0)
			{
				Entity p = m_Context->FindEntityByUUID(rl.Parent);
				if (p != Entity::Null)
				{
					auto* pt = m_Context->TryGetComponent<Component::Transform>(p);
					if (pt)
					{
						parentW = pt->WorldMatrix;
					}
				}
			}

			rt.WorldMatrix = parentW * rt.LocalMatrix;

			stack.clear();
			stack.push_back(root);

			while (!stack.empty())
			{
				Entity parent = stack.back();
				stack.pop_back();

				auto& parentT = m_Context->GetComponent<Component::Transform>(parent);
				auto& parentL = m_Context->GetComponent<Component::Link>(parent);

				for (UUID childId : parentL.Children)
				{
					Entity child = m_Context->FindEntityByUUID(childId);
					if (child == Entity::Null)
					{
						continue;
					}

					auto* childT = m_Context->TryGetComponent<Component::Transform>(child);
					auto* childL = m_Context->TryGetComponent<Component::Link>(child);
					if (!childT || !childL)
					{
						continue;
					}

					// Only update dirty children (should be true if you propagate dirty properly)
					if (childT->Static)
					{
						continue;
					}

					// If parent is dirty, child world must be recomputed.
					// Ensure child is considered dirty for this traversal.
					childT->Dirty = true;

					childT->WorldMatrix = parentT.WorldMatrix * childT->LocalMatrix;
					stack.push_back(child);
				}
			}
		}
	}

	void HierachySystem::MarkDirtyRecursive(Entity e)
	{
		auto* t = m_Context->TryGetComponent<Component::Transform>(e);
		auto* l = m_Context->TryGetComponent<Component::Link>(e);

		if (!t || !l)
		{
			return;
		}

		t->Dirty = true;

		for (UUID childId : l->Children)
		{
			Entity c = m_Context->FindEntityByUUID(childId);
			if (c != Entity::Null)
			{
				MarkDirtyRecursive(c);
			}
		}
	}

	void HierachySystem::ComputeWorldFromRoots()
	{
		std::vector<Entity> roots;
		roots.reserve(128);

		// Find roots (Parent == 0 or invalid parent)
		m_Context->Group<Component::Transform, Component::TransformEx, Component::Link>()
			.Each([&](Entity e, Component::Transform& t, Component::TransformEx& tEx, Component::Link& l)
			{
				if (l.Parent == 0)
				{
					roots.push_back(e);
					return;
				}

				Entity p = m_Context->FindEntityByUUID(l.Parent);
				if (p == Entity::Null)
				{
					roots.push_back(e);
				}
			});

		std::vector<Entity> stack;
		stack.reserve(256);

		for (Entity root : roots)
		{
			auto& rt = m_Context->GetComponent<Component::Transform>(root);
			auto& rl = m_Context->GetComponent<Component::Link>(root);

			// Root world == root local (unless you support scene root transform)
			rt.WorldMatrix = rt.LocalMatrix;

			stack.clear();
			stack.push_back(root);

			while (!stack.empty())
			{
				Entity parent = stack.back();
				stack.pop_back();

				auto& parentT = m_Context->GetComponent<Component::Transform>(parent);
				auto& parentL = m_Context->GetComponent<Component::Link>(parent);

				for (UUID childId : parentL.Children)
				{
					Entity child = m_Context->FindEntityByUUID(childId);
					if (child == Entity::Null)
					{
						continue;
					}

					auto& childT = m_Context->GetComponent<Component::Transform>(child);
					childT.WorldMatrix = parentT.WorldMatrix * childT.LocalMatrix;

					stack.push_back(child);
				}
			}
		}
	}

	void HierachySystem::OnUpdate2(float ts)
	{
		BEGIN_PROFILE_SYSTEM();

		m_Context->Group<Component::Transform, Component::TransformEx, Component::Link>()
			.Each([&](Entity entity, Component::Transform& transform, Component::TransformEx& transformEx, Component::Link& link)
			{
				const bool worldTChanged = (transformEx.PrevWorldTranslation != transform.WorldTranslation);
				const bool worldRChanged = (transformEx.PrevWorldRotation != transform.WorldRotation);
				const bool worldSChanged = (transformEx.PrevWorldScale != transform.WorldScale);

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

					transformEx.PrevWorldTranslation = transform.WorldTranslation;
					transformEx.PrevWorldRotation = transform.WorldRotation;
					transformEx.PrevWorldScale = transform.WorldScale;
				}

				// Calculate local matrices.
				if (!transform.Static)
				{
					transform.QRotation = glm::quat(glm::radians(transform.Rotation));
					transform.LocalMatrix = TRS(transform.Translation, transform.Rotation, transform.Scale);
				}
			});

		// Calculate world matrices.
		m_Context->Group<Component::Transform, Component::TransformEx, Component::Link>()
			.Each([&](Entity entity, Component::Transform& transform, Component::TransformEx& transformEx, Component::Link& link)
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

					transformEx.PrevWorldTranslation = transform.WorldTranslation;
					transformEx.PrevWorldRotation = transform.WorldRotation;
					transformEx.PrevWorldScale = transform.WorldScale;

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
					*childrenIterator = prevParentLink->Children.back();
					prevParentLink->Children.pop_back();
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
