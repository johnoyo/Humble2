#pragma once

#include "Resources\Types.h"
#include "Resources\Handle.h"
#include "Resources\ResourceManager.h"

#include "Core/Allocators.h"

#include "Utilities/Collections/Collections.h"
#include "Utilities/Collections/HashMap.h"
#include "Utilities/Allocators/BumpAllocator.h"

#include <functional>
#include <unordered_map>

namespace HBL2
{
	struct LocalDrawStream
	{
		Handle<Shader> Shader;
		Handle<Material> Material;
		uint64_t VariantHandle = 0;

		Handle<Buffer> IndexBuffer;
		Handle<Buffer> VertexBuffer;

		Handle<BindGroup> BindGroup;
		uint32_t Offset = 0;
		uint32_t Size = 0;

		uint32_t IndexCount = 0;
		uint32_t IndexOffset = 0;
		uint32_t VertexCount = 0;
		uint32_t VertexOffset = 0;
		uint32_t InstanceCount = 1;
		uint32_t InstanceOffset = 0;
	};

	struct GlobalDrawStream
	{
		Handle<BindGroup> BindGroup;

		uint32_t GlobalBufferSize = UINT32_MAX;
		uint32_t GlobalBufferOffset = UINT32_MAX;

		bool UsesDynamicOffset = false;
	};

	class DrawList
	{
	public:
		DrawList() = default;
		DrawList(Arena& arena, uint32_t reserveDrawCount);

		void Initialize(Arena& arena);
		void Initialize(Arena& arena, uint32_t reserveDrawCount);
		void Insert(LocalDrawStream&& draw);
		void Sort();
		void Reset();

		const uint32_t GetCount() const { return m_Draws.size(); }
		const Span<const LocalDrawStream> GetDraws() const { return { m_Draws.data(), m_Draws.size() }; }

	private:
		DArray<LocalDrawStream> m_Draws;
	};
}