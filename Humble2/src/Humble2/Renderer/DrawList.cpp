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

			if (shaderA != shaderB)
			{
				return shaderA < shaderB;
			}

			// Same shader -> sort by variant key
			return a.VariantHash.Key() < b.VariantHash.Key();
		});
	}

	void DrawList::Reset()
	{
		m_Draws.Clear();
	}
}
