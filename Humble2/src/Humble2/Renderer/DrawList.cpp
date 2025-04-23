#include "DrawList.h"

namespace HBL2
{
	void DrawList::Insert(const LocalDrawStream&& draw)
	{
#if 0
		m_Count++;
		m_Draws[draw.Shader.HashKey()].push_back(draw);
#else
		Material* mat = ResourceManager::Instance->GetMaterial(draw.Material);
		uint64_t hash = ResourceManager::Instance->GetShaderVariantHash(mat->VariantDescriptor);

		m_Draws[hash].push_back(draw);

		m_Count++;
#endif
	}

	void DrawList::Reset()
	{
		m_Count = 0;
		m_Draws.clear();
	}
}
