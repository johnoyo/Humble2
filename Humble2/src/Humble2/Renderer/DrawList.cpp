#include "DrawList.h"

namespace HBL2
{
	void DrawList::Insert(const LocalDrawStream&& draw)
	{
		m_Count++;
		m_Draws[draw.Shader.HashKey()].push_back(draw);
	}

	void DrawList::Reset()
	{
		m_Count = 0;
		m_Draws.clear();
	}
}
