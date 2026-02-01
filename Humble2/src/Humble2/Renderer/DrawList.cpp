#include "DrawList.h"

namespace HBL2
{
	void DrawList::Insert(LocalDrawStream&& draw)
	{
		m_Draws.Emplace(std::move(draw));
	}

	void DrawList::Sort()
	{
		std::sort(m_Draws.begin(), m_Draws.end(), [](const LocalDrawStream& a, const LocalDrawStream& b)
		{
			const uint64_t shaderA = a.Shader.HashKey();
			const uint64_t shaderB = b.Shader.HashKey();

			// First sort by shader.
			if (shaderA != shaderB)
			{
				return shaderA < shaderB;
			}

			// If they have same shader then sort by variant key.
			return a.VariantHandle < b.VariantHandle;
		});
	}

	void DrawList::Reset()
	{
		m_Draws.Clear();
	}
}
