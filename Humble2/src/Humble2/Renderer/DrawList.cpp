#include "DrawList.h"

namespace HBL2
{
	void DrawList::Insert(const LocalDrawStream&& draw)
	{
		m_Draws[draw.Shader.HashKey()].push_back(draw);
	}

	DrawList& DrawList::PerShader(std::function<void(LocalDrawStream&)> func)
	{
		m_PerShaderFunc = func;
		return *this;
	}

	DrawList& DrawList::PerDraw(std::function<void(LocalDrawStream&)> func)
	{
		m_PerDrawFunc = func;
		return *this;
	}

	void DrawList::Iterate()
	{
		for (auto&& [shaderID, drawList] : m_Draws)
		{
			m_PerShaderFunc(m_Draws[shaderID][0]);

			for (auto& draw : drawList)
			{
				m_PerDrawFunc(draw);
			}
		}
	}
}
