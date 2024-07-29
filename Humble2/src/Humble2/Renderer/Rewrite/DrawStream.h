#pragma once

#include "Types.h"
#include "Handle.h"

namespace HBL2
{
	struct LocalDrawStream
	{
		Handle<Shader> Shader;
		Handle<BindGroup> BindGroup;
		Handle<Mesh> Mesh;
	};

	struct GlobalDrawStream
	{
		Handle<BindGroup> BindGroup;
	};
}