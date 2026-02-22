#include "DrawList.h"

namespace HBL2
{
	DrawList::DrawList(Arena& arena, uint32_t reservedDrawCount)
	{
		Initialize(arena, reservedDrawCount);
	}

	DrawList::DrawList(ScratchArena& arena, uint32_t reservedDrawCount)
	{
		Initialize(arena, reservedDrawCount);
	}

	void DrawList::Initialize(Arena& arena)
	{
		m_Draws = MakeDArray<LocalDrawStream>(arena, 32768);
	}

	void DrawList::Initialize(ScratchArena& arena)
	{
		m_Draws = MakeDArray<LocalDrawStream>(arena, 32768);
	}

	void DrawList::Initialize(Arena& arena, uint32_t reservedDrawCount)
	{
		m_Draws = MakeDArray<LocalDrawStream>(arena, reservedDrawCount);
	}

	void DrawList::Initialize(ScratchArena& arena, uint32_t reservedDrawCount)
	{
		m_Draws = MakeDArray<LocalDrawStream>(arena, reservedDrawCount);
	}

	void DrawList::Insert(LocalDrawStream&& draw)
	{
		m_Draws.emplace_back(std::move(draw));
	}

	void DrawList::Sort()
	{
		std::sort(m_Draws.begin(), m_Draws.end(), [](const LocalDrawStream& a, const LocalDrawStream& b)
		{
			// First sort by shader.
			const uint32_t shaderA = a.Shader.HashKey();
			const uint32_t shaderB = b.Shader.HashKey();

			if (shaderA != shaderB)
			{
				return shaderA < shaderB;
			}

			// If they have same shader, then sort by variant key.
			if (a.VariantHandle != b.VariantHandle)
			{
				return a.VariantHandle < b.VariantHandle;
			}

			// Then, order by index buffer.
			const uint32_t ibA = a.IndexBuffer.HashKey();
			const uint32_t ibB = b.IndexBuffer.HashKey();

			return ibA < ibB;
		});
	}

	void DrawList::Reset()
	{
		m_Draws.clear();
	}
}
