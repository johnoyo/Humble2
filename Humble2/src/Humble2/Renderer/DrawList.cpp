#include "DrawList.h"

namespace HBL2
{
	void DrawList::Insert(const LocalDrawStream&& draw)
	{
		m_Draws[draw.Shader.HashKey()].push_back(draw);
	}
}
