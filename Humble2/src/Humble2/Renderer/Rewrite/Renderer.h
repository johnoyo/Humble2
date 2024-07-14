#pragma once

#include "Renderer\Rewrite\Handle.h"

namespace HBL
{
	class Mesh;
	class Material;
}

namespace HBL2
{
	class Renderer
	{
	public:
		static inline Renderer* Instance;

		virtual ~Renderer() = default;

		virtual void Initialize() = 0;
		virtual void BeginFrame() = 0;
		virtual void SetPipeline(HBL::Handle<HBL::Material>& material) = 0;
		virtual void SetBufferData(HBL::Handle<HBL::Mesh>& mesh) = 0;
		virtual void SetBuffers(HBL::Handle<HBL::Mesh>& mesh) = 0;
		virtual void SetBindGroups(HBL::Handle<HBL::Material>& material) = 0;
		virtual void Draw(HBL::Handle<HBL::Mesh>& mesh, HBL::Handle<HBL::Material>& material) = 0;
		virtual void DrawIndexed(HBL::Handle<HBL::Mesh>& mesh, HBL::Handle<HBL::Material>& material) = 0;
		virtual void EndFrame() = 0;
		virtual void Clean() = 0;
	};
}