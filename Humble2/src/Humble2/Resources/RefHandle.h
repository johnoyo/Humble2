#pragma once

#include "Handle.h"

#include <concepts>

namespace HBL2
{
	enum class ResourceType
	{
		None,
		Texture,
		Buffer,
		Shader,
		BindGroup,
		BindGroupLayout,
		RenderPass,
		RenderPassLayout,
		Mesh,
		Material,
		Scene,
		Script,
		Sound,
		Prefab,
	};

	template<typename T>
	struct PoolAccess
	{
		static void Acquire(Handle<T> handle);
		static void Release(Handle<T> handle);
	};

	template<typename T>
	class RefHandle
	{
	public:
		RefHandle() : m_Handle({ 0, 0 }) {}
		RefHandle(const RefHandle& other) : m_Handle(other.m_Handle)
		{
			if (m_Handle.IsValid())
			{
				PoolAccess<T>::Acquire(m_Handle);
			}
		}

		RefHandle(RefHandle&& other) noexcept : m_Handle(other.m_Handle)
		{
			other.m_Handle.Invalidate();
		}

		RefHandle(const Handle<T>& handle) : m_Handle(handle)
		{
			if (m_Handle.IsValid())
			{
				PoolAccess<T>::Acquire(m_Handle);
			}
		}

		RefHandle(Handle<T>&& handle) : m_Handle(handle)
		{
			handle.Invalidate();

			if (m_Handle.IsValid())
			{
				PoolAccess<T>::Acquire(m_Handle);
			}
		}

		~RefHandle()
		{
			if (m_Handle.IsValid())
			{
				PoolAccess<T>::Release(m_Handle);
			}
		}

		RefHandle& operator=(const RefHandle& other)
		{
			if (this == &other)
			{
				return *this;
			}

			if (m_Handle.IsValid())
			{
				PoolAccess<T>::Release(m_Handle);
			}

			m_Handle = other.m_Handle;

			if (m_Handle.IsValid())
			{
				PoolAccess<T>::Acquire(m_Handle);
			}

			return *this;
		}
		RefHandle& operator=(const Handle<T>& other)
		{
			if (m_Handle == other)
			{
				return *this;
			}

			if (m_Handle.IsValid())
			{
				PoolAccess<T>::Release(m_Handle);
			}

			m_Handle = other;

			if (m_Handle.IsValid())
			{
				PoolAccess<T>::Acquire(m_Handle);
			}

			return *this;
		}

		RefHandle& operator=(RefHandle&& other) noexcept
		{
			if (this == &other)
			{
				return *this;
			}

			if (m_Handle.IsValid())
			{
				PoolAccess<T>::Release(m_Handle);
			}

			m_Handle = other.m_Handle;

			other.m_Handle.Invalidate();

			return *this;
		}
		RefHandle& operator=(Handle<T>&& other) noexcept
		{
			if (m_Handle == other)
			{
				return *this;
			}

			if (m_Handle.IsValid())
			{
				PoolAccess<T>::Release(m_Handle);
			}

			m_Handle = other;

			other.Invalidate();

			if (m_Handle.IsValid())
			{
				PoolAccess<T>::Acquire(m_Handle);
			}

			return *this;
		}

		bool operator==(const RefHandle& other) const { return m_Handle == other.m_Handle; }
		bool operator!=(const RefHandle& other) const { return m_Handle != other.m_Handle; }

		bool IsValid() const
		{
			return m_Handle.IsValid();
		}

		void Release()
		{
			if (m_Handle.IsValid())
			{
				PoolAccess<T>::Release(m_Handle);
			}

			m_Handle.Invalidate();
		}

		const Handle<T> Get() const { return m_Handle; }

	private:
		RefHandle(uint16_t arrayIndex, uint16_t generationalCounter) : m_Handle({ arrayIndex, generationalCounter })
		{
			if (m_Handle.IsValid())
			{
				PoolAccess<T>::Acquire(m_Handle);
			}
		}

		Handle<T> m_Handle;

		template<typename U, typename H> friend class Pool;
		template<typename UH, typename UC, typename H> friend class SplitPool;
	};
}