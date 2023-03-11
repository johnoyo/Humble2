#pragma once

namespace HBL
{
	class VertexArray
	{
	public:
		virtual void Bind() = 0;
		virtual void UnBind() = 0;
	};
}