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
			return a.VariantHash < b.VariantHash;
		});
	}

	void DrawList::Reset()
	{
		m_Draws.Clear();
	}
}
