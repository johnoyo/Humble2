#pragma once

namespace HBL
{
	class IndexBuffer
	{
	public:
		virtual void Bind() = 0;
		virtual void UnBind() = 0;
	};
}